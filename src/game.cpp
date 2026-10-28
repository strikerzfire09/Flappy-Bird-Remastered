#include "game.hpp"

#include <algorithm>
#include <cmath>

#include "assets.hpp"
#include "config.hpp"
#include "daynight.hpp"
#include "fx.hpp"
#include "storage.hpp"
#include "ui.hpp"
#include "view.hpp"

#include <raylib.h>

namespace game {

Game gGame;

namespace {
constexpr float PIPE_W = 52.0f;
constexpr float PIPE_CAP_EXTRA = 6.0f; // cap extends beyond body
constexpr float PLAYABLE_H = cfg::WORLD_H - cfg::phys::GROUND_H;

float ClampGap(float score) {
    float gap = cfg::phys::PIPE_GAP_START -
                score * cfg::phys::PIPE_GAP_SHRINK_PER_POINT;
    return std::clamp(gap, cfg::phys::PIPE_GAP_MIN, cfg::phys::PIPE_GAP_START);
}

float PipeSpeed(float score) {
    return std::clamp(cfg::phys::PIPE_SPEED_START +
                          score * cfg::phys::PIPE_SPEED_ACCEL,
                      cfg::phys::PIPE_SPEED_START, cfg::phys::PIPE_SPEED_MAX);
}

// Bird collision box — matches sprite, slightly forgiving.
Rectangle BirdRect(const Bird& b) {
    float w = cfg::phys::BIRD_HITBOX_W;
    float h = cfg::phys::BIRD_HITBOX_H;
    return {b.pos.x - w * 0.5f, b.pos.y - h * 0.5f, w, h};
}

void PlayFlap() {
    if (cfg::audio::VOLUME_MASTER <= 0.0f) return;
    float pitch = cfg::audio::WING_PITCH_MIN +
                  static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f *
                      (cfg::audio::WING_PITCH_MAX - cfg::audio::WING_PITCH_MIN);
    SetSoundPitch(assets.sndWing, pitch);
    PlaySound(assets.sndWing);
}

// Draw a pipe piece correctly.
//   y      : top edge of this piece (0 for top pipe, bottomY for bottom)
//   h      : height of this piece
//   topPipe: true = cap sits at BOTTOM (gap end), body above
//            false = cap sits at TOP (gap end), body below
void DrawPipe(Texture2D& tex, float x, float y, float h, bool topPipe,
              Color tint) {
    float texW = static_cast<float>(tex.width);
    float texH = static_cast<float>(tex.height);

    // Cap source: top 30px of texture (the wider rim).
    // Body source: middle band of texture.
    Rectangle srcBody = {0.0f, 32.0f, texW, texH - 64.0f};

    float capH = std::clamp(h * 0.20f, 18.0f, 34.0f);
    float bodyH = h - capH;

    if (topPipe) {
        if (bodyH > 0) {
            Rectangle dst = {x, y, PIPE_W, bodyH};
            DrawTexturePro(tex, srcBody, dst, {0, 0}, 0.0f, tint);
        }
        if (capH > 0) {
            Rectangle src = {0.0f, 0.0f, texW, -30.0f};
            Rectangle dst = {x - PIPE_CAP_EXTRA, y + bodyH - PIPE_CAP_EXTRA,
                             PIPE_W + PIPE_CAP_EXTRA * 2, capH + PIPE_CAP_EXTRA};
            DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, tint);
        }
    } else {
        if (capH > 0) {
            Rectangle src = {0.0f, 0.0f, texW, 30.0f};
            Rectangle dst = {x - PIPE_CAP_EXTRA, y - PIPE_CAP_EXTRA,
                             PIPE_W + PIPE_CAP_EXTRA * 2,
                             capH + PIPE_CAP_EXTRA};
            DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, tint);
        }
        if (bodyH > 0) {
            Rectangle dst = {x, y + capH, PIPE_W, bodyH};
            DrawTexturePro(tex, srcBody, dst, {0, 0}, 0.0f, tint);
        }
    }
}
} // namespace

void Game::Reset() {
    state = START;
    score = 0;
    pipeTimer = cfg::phys::PIPE_SPAWN_INTERVAL;
    groundScroll = 0.0f;
    runTime = 0.0f;
    deathTimer = 0.0f;
    bird = Bird{};
    for (auto& p : pipes) p = Pipe{};
    fx::gFx.Reset();
    daynightSt = daynight::State{};

    SetRandomSeed(static_cast<unsigned int>(GetTime() * 1000000.0));

    // First pipe ahead with random height.
    pipes[0].x = cfg::WORLD_W + 160.0f;
    pipes[0].spawned = true;
    pipes[0].topH = static_cast<float>(GetRandomValue(55, 250));
    pipes[0].bottomY = pipes[0].topH + ClampGap(0.0f);

    for (int i = 1; i < 6; ++i) pipes[i].spawned = false;

    highScore = storage::LoadHighScore();
    isNewBest = false;
    flapPressed = false;
    pausePressed = false;
    restartPressed = false;
    menuPressed = false;
}

void Game::Die() {
    if (state != PLAYING) return;
    state = DYING;
    deathTimer = 0.0f;
    PlaySound(assets.sndHit);
    PlaySound(assets.sndDie);
    fx::TriggerShake(12.0f);
    fx::TriggerFlash(1.0f);
    fx::SpawnDeathExplosion(bird.pos);
}

void Game::SpawnPipe() {
    int slot = -1;
    for (int i = 0; i < 6; ++i) {
        if (!pipes[i].spawned) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    // Spawn far right with min spacing from any active pipe.
    float spawnX = cfg::WORLD_W + 170.0f;
    for (auto& other : pipes) {
        if (other.spawned && other.x > spawnX - 15.0f) {
            spawnX = other.x + cfg::phys::PIPE_MIN_X_FROM_EDGE;
        }
    }

    float gap = ClampGap(static_cast<float>(score));
    float minTop = 50.0f;
    float maxTop = PLAYABLE_H - gap - 55.0f;
    if (maxTop < minTop) maxTop = minTop + 15.0f;

    float topH = static_cast<float>(GetRandomValue(
        static_cast<int>(minTop), static_cast<int>(maxTop)));

    Pipe& p = pipes[slot];
    p.x = spawnX;
    p.topH = topH;
    p.bottomY = topH + gap;
    p.passed = false;
    p.spawned = true;
}

void Game::UpdatePipes(float dt) {
    float speed = PipeSpeed(static_cast<float>(score));

    pipeTimer -= dt;
    while (pipeTimer <= 0.0f) {
        SpawnPipe();
        pipeTimer += cfg::phys::PIPE_SPAWN_INTERVAL;
    }

    for (auto& p : pipes) {
        if (!p.spawned) continue;
        p.x -= speed * dt;

        if (!p.passed && gGame.bird.pos.x > p.x + PIPE_W * 0.5f) {
            p.passed = true;
            score++;
            PlaySound(assets.sndPoint);
            fx::SpawnScoreSparkle({gGame.bird.pos.x, gGame.bird.pos.y});
            fx::TriggerShake(2.0f);
            if (score > highScore) {
                highScore = score;
                isNewBest = true;
            }
        }

        if (p.x < -PIPE_W - 30.0f) {
            p.spawned = false;
            p.x = cfg::WORLD_W + 170.0f;
        }
    }
}

void Game::UpdateBird(float dt) {
    if (state == START) {
        bird.pos.y = cfg::WORLD_H / 2.0f + std::sinf(runTime * 3.0f) * 5.0f;
        bird.rotation = std::sinf(runTime * 3.0f) * 6.0f;
        bird.flapAnim += dt * 5.0f;
        if (bird.flapAnim > 1.0f) bird.flapAnim -= 1.0f;
        return;
    }

    bird.vel.y += cfg::phys::GRAVITY * dt;
    if (bird.vel.y > cfg::phys::MAX_FALL_SPEED)
        bird.vel.y = cfg::phys::MAX_FALL_SPEED;
    bird.pos.y += bird.vel.y * dt;

    if (flapPressed) {
        Flap();
        flapPressed = false;
    }

    float targetRot = (bird.vel.y < -40.0f) ? -25.0f : 60.0f;
    bird.rotation += (targetRot - bird.rotation) * dt * 10.0f;
    bird.rotation = std::clamp(bird.rotation, -30.0f, 75.0f);

    bird.flapAnim += dt * 12.0f;
    if (bird.flapAnim > 1.0f) bird.flapAnim -= 1.0f;

    float groundY = cfg::WORLD_H - cfg::phys::GROUND_H;
    if (bird.pos.y + cfg::phys::BIRD_RADIUS >= groundY) {
        bird.pos.y = groundY - cfg::phys::BIRD_RADIUS;
        bird.grounded = true;
        bird.vel.y = 0.0f;
        if (state == PLAYING) Die();
    } else if (bird.pos.y - cfg::phys::BIRD_RADIUS <= 0.0f) {
        bird.pos.y = cfg::phys::BIRD_RADIUS;
        bird.vel.y = 0.0f;
        if (state == PLAYING) Die();
    }
}

void Game::Flap() {
    if (state == PLAYING) {
        bird.vel.y = cfg::phys::FLAP_VELOCITY;
        bird.grounded = false;
        PlayFlap();
        fx::SpawnDust(bird.pos, 8);
        fx::TriggerShake(3.5f);
    }
}

void Game::CheckCollisions() {
    if (state != PLAYING) return;
    Rectangle br = BirdRect(bird);
    for (auto& p : pipes) {
        if (!p.spawned) continue;
        // Collision box matches the visible pipe body (exclude cap overhang).
        Rectangle top = {p.x + 3.0f, 0.0f, PIPE_W - 6.0f, p.topH};
        Rectangle bottom = {p.x + 3.0f, p.bottomY, PIPE_W - 6.0f,
                            PLAYABLE_H - p.bottomY};
        if (CheckCollisionRecs(br, top) || CheckCollisionRecs(br, bottom)) {
            Die();
            return;
        }
    }
}

void GameInit() { gGame.Reset(); }

void GameFixedUpdate() {
    Game& g = gGame;
    bool flap = g.flapPressed;
    g.flapPressed = false;

    if (g.pausePressed) {
        if (g.state == PLAYING) g.state = PAUSED;
        else if (g.state == PAUSED) g.state = PLAYING;
        g.pausePressed = false;
    }

    if (g.state == PAUSED) {
        if (g.restartPressed || g.menuPressed) {
            g.Reset();
            g.state = START;
        }
        g.restartPressed = false;
        g.menuPressed = false;
        return;
    }

    fx::gFx.Update(static_cast<float>(cfg::FIXED_DT));

    if (g.state == START) {
        g.runTime += cfg::FIXED_DT;
        g.UpdateBird(static_cast<float>(cfg::FIXED_DT));
        if (flap) {
            g.state = PLAYING;
            g.bird.vel.y = cfg::phys::FLAP_VELOCITY;
            PlayFlap();
            PlaySound(assets.sndSwoosh);
            fx::SpawnDust(g.bird.pos, 8);
        }
        return;
    }

    if (g.state == PLAYING) {
        g.runTime += cfg::FIXED_DT;

        g.groundScroll -=
            PipeSpeed(static_cast<float>(g.score)) * cfg::FIXED_DT;
        float baseW = static_cast<float>(assets.base.width);
        if (baseW > 0.0f && g.groundScroll <= -baseW) g.groundScroll += baseW;

        g.flapPressed = flap;
        g.UpdateBird(static_cast<float>(cfg::FIXED_DT));
        g.UpdatePipes(static_cast<float>(cfg::FIXED_DT));
        g.CheckCollisions();

        daynight::Update(g.daynightSt, g.score,
                         static_cast<float>(cfg::FIXED_DT));
        return;
    }

    if (g.state == DYING) {
        g.runTime += cfg::FIXED_DT;
        g.deathTimer += cfg::FIXED_DT;
        g.UpdateBird(static_cast<float>(cfg::FIXED_DT));
        if (g.bird.grounded && g.deathTimer > 0.55f) {
            g.state = GAMEOVER;
            if (g.score > storage::LoadHighScore()) {
                storage::SaveHighScore(g.score);
                g.highScore = g.score;
                g.isNewBest = true;
            }
        }
        return;
    }

    if (g.state == GAMEOVER) {
        g.runTime += cfg::FIXED_DT;
        if (g.restartPressed) {
            g.Reset();
            g.state = PLAYING;
            g.bird.vel.y = cfg::phys::FLAP_VELOCITY;
            PlayFlap();
        } else if (g.menuPressed) {
            g.Reset();
            g.state = START;
        }
        g.restartPressed = false;
        g.menuPressed = false;
        return;
    }

    // Drop stale edges if they were set in a state that doesn't consume them.
    g.restartPressed = false;
    g.menuPressed = false;
}

void GameRender() {
    Game& g = gGame;
    view::Layout layout = view::Compute();
    Camera2D cam = layout.Camera();

    Vector2 shake = {0.0f, 0.0f};
    fx::gFx.DrawShakeOffset(shake);
    cam.offset.x += shake.x * layout.scale;
    cam.offset.y += shake.y * layout.scale;

    Vector2 worldMouse = layout.ToWorld(GetMousePosition());

    // Letterbox bars outside the world.
    ClearBackground(BLACK);

    BeginMode2D(cam);

    Color sky = daynight::SkyColor(g.daynightSt);
    DrawRectangle(0, 0, cfg::WORLD_W, cfg::WORLD_H, sky);

    Texture2D& bg = daynight::IsNight(g.daynightSt) ? assets.bgNight
                                                    : assets.bgDay;
    if (bg.id != 0) {
        Rectangle src = {0, 0, static_cast<float>(bg.width),
                         static_cast<float>(bg.height)};
        Rectangle dst = {0, 0, static_cast<float>(cfg::WORLD_W),
                         static_cast<float>(cfg::WORLD_H)};
        DrawTexturePro(bg, src, dst, {0, 0}, 0.0f, WHITE);
    }

    float fadeAlpha = (g.daynightSt.phase == daynight::FADING_TO_NIGHT)
                          ? daynight::Ease(g.daynightSt)
                          : (g.daynightSt.phase == daynight::NIGHT ? 1.0f : 0.0f);
    daynight::DrawScenery(g.daynightSt, fadeAlpha);

    Color pipeTint = daynight::PipeTint(g.daynightSt);
    Texture2D& pipeTex = daynight::IsNight(g.daynightSt) ? assets.pipeBodyNight
                                                         : assets.pipeBody;
    for (auto& p : g.pipes) {
        if (!p.spawned || pipeTex.id == 0) continue;
        DrawPipe(pipeTex, p.x, 0.0f, p.topH, true, pipeTint);
        DrawPipe(pipeTex, p.x, p.bottomY, PLAYABLE_H - p.bottomY, false,
                 pipeTint);
    }

    if (assets.base.id != 0) {
        float baseW = static_cast<float>(assets.base.width);
        float baseH = static_cast<float>(assets.base.height);
        Rectangle bsrc = {0, 0, baseW, baseH};
        float gx = g.groundScroll;
        while (gx < cfg::WORLD_W) {
            Rectangle bdst = {gx, cfg::WORLD_H - baseH, baseW, baseH};
            DrawTexturePro(assets.base, bsrc, bdst, {0, 0}, 0.0f, WHITE);
            gx += baseW;
        }
    }
    DrawLine(0, cfg::WORLD_H - cfg::phys::GROUND_H, cfg::WORLD_W,
             cfg::WORLD_H - cfg::phys::GROUND_H, cfg::vis::GROUND_LINE);

    {
        int frame = static_cast<int>(g.bird.flapAnim * 3.0f) % 3;
        Texture2D* birdTex = &assets.birdMid;
        if (frame == 0) birdTex = &assets.birdUp;
        else if (frame == 2) birdTex = &assets.birdDown;
        if (birdTex->id != 0) {
            Rectangle bsrc = {0, 0, static_cast<float>(birdTex->width),
                              static_cast<float>(birdTex->height)};
            Rectangle bdst = {g.bird.pos.x, g.bird.pos.y,
                              static_cast<float>(birdTex->width),
                              static_cast<float>(birdTex->height)};
            Vector2 origin = {bdst.width * 0.5f, bdst.height * 0.5f};
            DrawTexturePro(*birdTex, bsrc, bdst, origin, g.bird.rotation,
                           WHITE);
        }
    }

    if (g.state == PLAYING) {
        ui::DrawHud(g.score, g.highScore);
        if (g.daynightSt.justStartedFade) {
            ui::DrawNightfallToast(cfg::WORLD_W / 2.0f, 160.0f,
                                   daynight::Ease(g.daynightSt));
        }
    }

    fx::gFx.DrawParticles();
    fx::gFx.DrawFlash();

    static ui::Button btnStart{{cfg::WORLD_W / 2.0f - 70.0f,
                                cfg::WORLD_H / 2.0f + 60.0f, 140.0f, 34.0f},
                               "START"};
    static ui::Button btnRestart{{0, 0, 140, 36}, "RESTART"};
    static ui::Button btnMenu{{0, 0, 140, 36}, "MAIN MENU"};
    static ui::Button btnResume{{0, 0, 140, 34}, "RESUME"};

    if (g.state == START) {
        ui::DrawStartScreen(g.highScore);
        ui::UpdateButton(btnStart, worldMouse);
        ui::DrawButton(btnStart);
        if (ui::ButtonPressed(btnStart)) {
            g.state = PLAYING;
            g.bird.vel.y = cfg::phys::FLAP_VELOCITY;
            PlayFlap();
            PlaySound(assets.sndSwoosh);
            fx::SpawnDust(g.bird.pos, 8);
        }
    } else if (g.state == GAMEOVER) {
        ui::DrawGameOver(g.score, g.highScore, g.isNewBest);
        btnRestart.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 318.0f, 140.0f, 36.0f};
        btnMenu.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 364.0f, 140.0f, 36.0f};
        ui::UpdateButton(btnRestart, worldMouse);
        ui::UpdateButton(btnMenu, worldMouse);
        ui::DrawButton(btnRestart);
        ui::DrawButton(btnMenu);
        if (ui::ButtonPressed(btnRestart)) {
            g.Reset();
            g.state = PLAYING;
            g.bird.vel.y = cfg::phys::FLAP_VELOCITY;
            PlayFlap();
        }
        if (ui::ButtonPressed(btnMenu)) {
            g.Reset();
            g.state = START;
        }
    } else if (g.state == PAUSED) {
        ui::DrawPause(g.score);
        btnResume.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 236.0f, 140.0f, 34.0f};
        btnRestart.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 280.0f, 140.0f, 34.0f};
        btnMenu.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 324.0f, 140.0f, 34.0f};
        ui::UpdateButton(btnResume, worldMouse);
        ui::UpdateButton(btnRestart, worldMouse);
        ui::UpdateButton(btnMenu, worldMouse);
        ui::DrawButton(btnResume);
        ui::DrawButton(btnRestart);
        ui::DrawButton(btnMenu);
        if (ui::ButtonPressed(btnResume)) g.state = PLAYING;
        if (ui::ButtonPressed(btnRestart)) {
            g.Reset();
            g.state = START;
        }
        if (ui::ButtonPressed(btnMenu)) {
            g.Reset();
            g.state = START;
        }
    }

    EndMode2D();
}

} // namespace game
