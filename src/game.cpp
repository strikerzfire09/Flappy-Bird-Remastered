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
constexpr float PIPE_CAP_EXTRA = 6.0f; // wider rim sticks out each side
constexpr float PIPE_CAP_MIN_H = 18.0f;
constexpr float PIPE_CAP_MAX_H = 34.0f;
constexpr float PLAYABLE_H = cfg::WORLD_H - cfg::phys::GROUND_H;
constexpr float MIN_TOP = 48.0f;
constexpr float MIN_BOTTOM = 48.0f;
constexpr float SPAWN_X = cfg::WORLD_W + 56.0f;

ui::Button gRestartButton{{74, 314, 140, 36}, "RESTART"};
ui::Button gMenuButton{{74, 358, 140, 36}, "MAIN MENU"};
ui::Button gResumeButton{{74, 226, 140, 36}, "RESUME"};

float ClampGap(float score) {
    float gap = cfg::phys::PIPE_GAP_START -
                score * cfg::phys::PIPE_GAP_SHRINK_PER_POINT;
    return std::clamp(gap, cfg::phys::PIPE_GAP_MIN,
                      cfg::phys::PIPE_GAP_START);
}

float PipeSpeed(float score) {
    return std::clamp(cfg::phys::PIPE_SPEED_START +
                          score * cfg::phys::PIPE_SPEED_ACCEL,
                      cfg::phys::PIPE_SPEED_START,
                      cfg::phys::PIPE_SPEED_MAX);
}

float PipeCapHeight(float height) {
    return std::clamp(height * 0.20f, PIPE_CAP_MIN_H, PIPE_CAP_MAX_H);
}

// Bird sprite is 34x24. The collision box is deliberately a little smaller
// than the art, but centred on the sprite so it feels honest.
Rectangle BirdRect(const Bird& b) {
    const float w = cfg::phys::BIRD_HITBOX_W;
    const float h = cfg::phys::BIRD_HITBOX_H;
    return {b.pos.x - w * 0.5f, b.pos.y - h * 0.5f, w, h};
}

void PlayFlap() {
    const float pitch =
        cfg::audio::WING_PITCH_MIN +
        (static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f) *
            (cfg::audio::WING_PITCH_MAX - cfg::audio::WING_PITCH_MIN);
    SetSoundPitch(assets.sndWing, pitch);
    PlaySound(assets.sndWing);
}

// Draw one pipe piece. `y` is the top edge and `h` its full height. The wider
// cap sits at the gap end, exactly where the collision box puts it.
void DrawPipe(Texture2D& tex, float x, float y, float h, bool topPipe,
              Color tint) {
    if (h <= 1.0f) return;

    const float texW = static_cast<float>(tex.width);
    const float texH = static_cast<float>(tex.height);
    Rectangle srcBody = {0.0f, 32.0f, texW, texH - 64.0f};

    float capH = PipeCapHeight(h);
    if (capH > h) capH = h;
    const float bodyH = h - capH;

    if (topPipe) {
        // Body runs from the ceiling to the cap.
        if (bodyH > 0.0f) {
            DrawTexturePro(tex, srcBody, {x, y, PIPE_W, bodyH}, {0, 0}, 0.0f,
                           tint);
        }
        // Cap at the gap end, flipped so the rim faces down.
        Rectangle srcCap = {0.0f, 0.0f, texW, -30.0f};
        Rectangle dst = {x - PIPE_CAP_EXTRA, y + bodyH - PIPE_CAP_EXTRA,
                         PIPE_W + PIPE_CAP_EXTRA * 2.0f,
                         capH + PIPE_CAP_EXTRA};
        DrawTexturePro(tex, srcCap, dst, {0, 0}, 0.0f, tint);
    } else {
        // Cap at the top (gap end), then the body below it.
        Rectangle srcCap = {0.0f, 0.0f, texW, 30.0f};
        Rectangle dst = {x - PIPE_CAP_EXTRA, y - PIPE_CAP_EXTRA,
                         PIPE_W + PIPE_CAP_EXTRA * 2.0f,
                         capH + PIPE_CAP_EXTRA};
        DrawTexturePro(tex, srcCap, dst, {0, 0}, 0.0f, tint);
        if (bodyH > 0.0f) {
            DrawTexturePro(tex, srcBody, {x, y + capH, PIPE_W, bodyH},
                           {0, 0}, 0.0f, tint);
        }
    }
}

void DrawPipeHitboxes(const Pipe& p) {
    const float topCap = PipeCapHeight(p.topH);
    const float bottomH = PLAYABLE_H - p.bottomY;
    const float bottomCap = PipeCapHeight(bottomH);

    DrawRectangleLinesEx(
        {p.x, 0.0f, PIPE_W, std::max(0.0f, p.topH - topCap)}, 1.0f, YELLOW);
    DrawRectangleLinesEx(
        {p.x - PIPE_CAP_EXTRA, p.topH - topCap - PIPE_CAP_EXTRA,
         PIPE_W + PIPE_CAP_EXTRA * 2.0f, topCap + PIPE_CAP_EXTRA},
        1.0f, ORANGE);
    DrawRectangleLinesEx(
        {p.x - PIPE_CAP_EXTRA, p.bottomY - PIPE_CAP_EXTRA,
         PIPE_W + PIPE_CAP_EXTRA * 2.0f, bottomCap + PIPE_CAP_EXTRA},
        1.0f, ORANGE);
    DrawRectangleLinesEx(
        {p.x, p.bottomY + bottomCap, PIPE_W,
         std::max(0.0f, bottomH - bottomCap)},
        1.0f, YELLOW);
}

void LayoutGameOverButtons() {
    gRestartButton.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 314.0f, 140.0f,
                             36.0f};
    gMenuButton.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 358.0f, 140.0f, 36.0f};
}

void LayoutPauseButtons() {
    gResumeButton.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 226.0f, 140.0f,
                            36.0f};
    gRestartButton.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 268.0f, 140.0f,
                             36.0f};
    gMenuButton.bounds = {cfg::WORLD_W / 2.0f - 70.0f, 310.0f, 140.0f, 36.0f};
}

} // namespace

void Game::Reset() {
    state = START;
    score = 0;
    isNewBest = false;
    runTime = 0.0f;
    deathTimer = 0.0f;
    groundScroll = 0.0f;
    bird = Bird{};

    for (auto& p : pipes) p = Pipe{};
    fx::gFx.Reset();
    daynightSt = daynight::State{};

    flapPressed = false;
    pausePressed = false;
    restartPressed = false;
    menuPressed = false;

    SetRandomSeed(static_cast<unsigned int>(GetTime() * 1000000.0));

    Pipe& first = pipes[0];
    first.spawned = true;
    first.x = SPAWN_X;
    const float gap = ClampGap(0.0f);
    const float maxTop = PLAYABLE_H - gap - MIN_BOTTOM;
    first.topH = static_cast<float>(GetRandomValue(
        static_cast<int>(MIN_TOP), static_cast<int>(maxTop)));
    first.bottomY = first.topH + gap;
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

    float rightmost = -100000.0f;
    float previousTop = -1.0f;
    for (const auto& p : pipes) {
        if (!p.spawned) continue;
        if (p.x > rightmost) {
            rightmost = p.x;
            previousTop = p.topH;
        }
    }

    // Distance-based spawning guarantees the same horizontal gap at every
    // speed, so pipes can never bunch up or overlap.
    const float x = (rightmost < 0.0f) ? SPAWN_X
                                       : rightmost + cfg::phys::PIPE_SPACING;

    const float gap = ClampGap(static_cast<float>(score));
    const float maxTop = PLAYABLE_H - gap - MIN_BOTTOM;
    float minTop = MIN_TOP;
    float maxTopClamped = std::max(minTop + 1.0f, maxTop);

    // Gentle random walk: varied, but never an impossible vertical jump.
    if (previousTop >= 0.0f) {
        minTop = std::max(minTop, previousTop - 82.0f);
        maxTopClamped = std::min(maxTopClamped, previousTop + 82.0f);
        if (maxTopClamped <= minTop) {
            minTop = MIN_TOP;
            maxTopClamped = std::max(minTop + 1.0f, maxTop);
        }
    }

    Pipe& p = pipes[slot];
    p.x = x;
    p.topH = static_cast<float>(GetRandomValue(
        static_cast<int>(minTop), static_cast<int>(maxTopClamped)));
    p.bottomY = p.topH + gap;
    p.passed = false;
    p.spawned = true;
}

void Game::UpdatePipes(float dt) {
    const float speed = PipeSpeed(static_cast<float>(score));

    for (auto& p : pipes) {
        if (!p.spawned) continue;
        p.x -= speed * dt;

        if (!p.passed && bird.pos.x > p.x + PIPE_W * 0.5f) {
            p.passed = true;
            ++score;
            PlaySound(assets.sndPoint);
            fx::SpawnScoreSparkle(bird.pos);
            fx::TriggerShake(2.0f);
            if (score > highScore) {
                highScore = score;
                isNewBest = true;
            }
        }

        if (p.x < -PIPE_W - 24.0f) {
            p.spawned = false;
        }
    }

    float rightmost = -100000.0f;
    for (const auto& p : pipes) {
        if (p.spawned && p.x > rightmost) rightmost = p.x;
    }
    if (rightmost < cfg::WORLD_W + PIPE_W - cfg::phys::PIPE_SPACING) {
        SpawnPipe();
    }
}

void Game::UpdateBird(float dt) {
    // The art is 34x24; clamp the sprite (not the smaller hitbox) to the
    // ground so the bird visually rests on top of it.
    constexpr float spriteHalfH = 12.0f;

    if (state == START) {
        bird.pos.y = cfg::WORLD_H * 0.5f + std::sin(runTime * 3.0f) * 5.0f;
        bird.rotation = std::sin(runTime * 3.0f) * 6.0f;
        bird.flapAnim += dt * 5.0f;
        if (bird.flapAnim > 1.0f) bird.flapAnim -= 1.0f;
        return;
    }

    bird.vel.y += cfg::phys::GRAVITY * dt;
    if (bird.vel.y > cfg::phys::MAX_FALL_SPEED)
        bird.vel.y = cfg::phys::MAX_FALL_SPEED;
    bird.pos.y += bird.vel.y * dt;

    const float targetRot = (bird.vel.y < -40.0f) ? -25.0f : 60.0f;
    bird.rotation += (targetRot - bird.rotation) * dt * 10.0f;
    bird.rotation = std::clamp(bird.rotation, -30.0f, 75.0f);

    bird.flapAnim += dt * 12.0f;
    if (bird.flapAnim > 1.0f) bird.flapAnim -= 1.0f;

    const float groundY = cfg::WORLD_H - cfg::phys::GROUND_H;
    if (bird.pos.y + spriteHalfH >= groundY) {
        bird.pos.y = groundY - spriteHalfH;
        bird.grounded = true;
        if (state == PLAYING) Die();
    } else if (bird.pos.y - spriteHalfH <= 0.0f) {
        bird.pos.y = spriteHalfH;
        bird.vel.y = 0.0f;
        if (state == PLAYING) Die();
    }
}

void Game::CheckCollisions() {
    if (state != PLAYING) return;

    const Rectangle br = BirdRect(bird);
    for (const auto& p : pipes) {
        if (!p.spawned) continue;

        const float topCap = PipeCapHeight(p.topH);
        const float bottomH = PLAYABLE_H - p.bottomY;
        const float bottomCap = PipeCapHeight(bottomH);

        const Rectangle topBody = {p.x, 0.0f, PIPE_W,
                                   std::max(0.0f, p.topH - topCap)};
        const Rectangle topCapRect = {
            p.x - PIPE_CAP_EXTRA, p.topH - topCap - PIPE_CAP_EXTRA,
            PIPE_W + PIPE_CAP_EXTRA * 2.0f, topCap + PIPE_CAP_EXTRA};
        const Rectangle bottomCapRect = {
            p.x - PIPE_CAP_EXTRA, p.bottomY - PIPE_CAP_EXTRA,
            PIPE_W + PIPE_CAP_EXTRA * 2.0f, bottomCap + PIPE_CAP_EXTRA};
        const Rectangle bottomBody = {
            p.x, p.bottomY + bottomCap, PIPE_W,
            std::max(0.0f, bottomH - bottomCap)};

        if (CheckCollisionRecs(br, topBody) ||
            CheckCollisionRecs(br, topCapRect) ||
            CheckCollisionRecs(br, bottomCapRect) ||
            CheckCollisionRecs(br, bottomBody)) {
            Die();
            return;
        }
    }
}

void Game::Flap() {
    if (state != PLAYING) return;
    bird.vel.y = cfg::phys::FLAP_VELOCITY;
    bird.grounded = false;
    PlayFlap();
    fx::SpawnDust(bird.pos, 8);
    fx::TriggerShake(3.0f);
}

void Game::Die() {
    if (state != PLAYING) return;
    state = DYING;
    deathTimer = 0.0f;
    PlaySound(assets.sndHit);
    PlaySound(assets.sndDie);
    fx::TriggerShake(12.0f);
    fx::TriggerFlash(0.85f);
    fx::SpawnDeathExplosion(bird.pos);
    storage::SaveHighScore(highScore);
}

void GameInit() {
    gGame.highScore = storage::LoadHighScore();
    gGame.Reset();
}

void GameFixedUpdate() {
    Game& g = gGame;
    const float dt = cfg::FIXED_DT;

    // Consume each edge exactly once per frame, never once per physics step.
    const bool flap = g.flapPressed;
    const bool pause = g.pausePressed;
    const bool restart = g.restartPressed;
    const bool menu = g.menuPressed;
    g.flapPressed = false;
    g.pausePressed = false;
    g.restartPressed = false;
    g.menuPressed = false;

    if (pause) {
        if (g.state == PLAYING) {
            g.state = PAUSED;
            return;
        }
        if (g.state == PAUSED) {
            g.state = PLAYING;
            return;
        }
        // In other states ESC is only the menu key; fall through so
        // GAMEOVER can handle `menu` below.
    }

    if (g.state == PAUSED) {
        if (restart) g.Reset();
        return;
    }

    if (restart && g.state != START) {
        g.Reset();
        return;
    }

    const auto scrollGround = [&](float speed) {
        g.groundScroll -= speed * dt;
        const float baseW = static_cast<float>(assets.base.width);
        if (baseW > 0.0f && g.groundScroll <= -baseW) g.groundScroll += baseW;
    };

    daynight::Update(g.daynightSt,
                     g.state == PLAYING ? g.score : 0, dt);
    fx::gFx.Update(dt);

    if (g.state == START) {
        g.runTime += dt;
        scrollGround(28.0f);
        g.UpdateBird(dt);
        if (flap) {
            g.state = PLAYING;
            g.Flap();
            PlaySound(assets.sndSwoosh);
        }
        return;
    }

    if (g.state == PLAYING) {
        g.runTime += dt;

        const float speed = PipeSpeed(static_cast<float>(g.score));
        scrollGround(speed);

        if (flap) g.Flap();
        g.UpdateBird(dt);
        if (g.state != PLAYING) return;

        g.UpdatePipes(dt);
        g.CheckCollisions();
        return;
    }

    if (g.state == DYING) {
        g.runTime += dt;
        g.deathTimer += dt;
        g.UpdateBird(dt);
        if (g.bird.grounded && g.deathTimer > 0.55f) {
            g.state = GAMEOVER;
            storage::SaveHighScore(g.highScore);
        }
        return;
    }

    if (g.state == GAMEOVER) {
        g.runTime += dt;
        if (menu) {
            g.Reset();
        }
        return;
    }
}

void GameFrameInput(Vector2 worldMouse) {
    Game& g = gGame;

    if (g.state == GAMEOVER) {
        LayoutGameOverButtons();
        ui::UpdateButton(gRestartButton, worldMouse);
        ui::UpdateButton(gMenuButton, worldMouse);
        if (ui::ButtonPressed(gRestartButton) ||
            ui::ButtonPressed(gMenuButton)) {
            g.Reset();
        }
        return;
    }

    if (g.state == PAUSED) {
        LayoutPauseButtons();
        ui::UpdateButton(gResumeButton, worldMouse);
        ui::UpdateButton(gRestartButton, worldMouse);
        ui::UpdateButton(gMenuButton, worldMouse);
        if (ui::ButtonPressed(gResumeButton)) {
            g.state = PLAYING;
        } else if (ui::ButtonPressed(gRestartButton) ||
                   ui::ButtonPressed(gMenuButton)) {
            g.Reset();
        }
    }
}

void GameRender() {
    Game& g = gGame;

    const view::Layout layout = view::Compute();
    Camera2D cam = layout.Camera();

    Vector2 shake = {0.0f, 0.0f};
    fx::gFx.DrawShakeOffset(shake);
    cam.offset.x += shake.x * layout.scale;
    cam.offset.y += shake.y * layout.scale;

    const float night = daynight::Ease(g.daynightSt);
    const auto alpha = [](float v) {
        v = std::clamp(v, 0.0f, 1.0f);
        return static_cast<unsigned char>(v * 255.0f + 0.5f);
    };
    const auto mix = [](Color a, Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color{
            static_cast<unsigned char>(a.r + (b.r - a.r) * t),
            static_cast<unsigned char>(a.g + (b.g - a.g) * t),
            static_cast<unsigned char>(a.b + (b.b - a.b) * t), 255};
    };

    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const float baseW = static_cast<float>(assets.base.width);
    const float baseH = static_cast<float>(assets.base.height);
    const Color groundTint = {
        static_cast<unsigned char>(255.0f - 55.0f * night),
        static_cast<unsigned char>(255.0f - 42.0f * night),
        static_cast<unsigned char>(255.0f - 22.0f * night), 255};

    // ----- letterbox backdrop -----
    // Fill the whole window so wide/tall resizes never expose stark black
    // bars. The sky art is continued horizontally into the side panels, and
    // the scrolling ground strip continues seamlessly as well.
    DrawRectangleGradientV(
        0, 0, screenW, screenH,
        mix({70, 180, 198, 255}, {6, 12, 30, 255}, night),
        mix({214, 238, 226, 255}, {22, 40, 60, 255}, night));

    const float worldW = static_cast<float>(cfg::WORLD_W) * layout.scale;
    if (worldW > 0.0f) {
        float startX = layout.offset.x;
        while (startX > 0.0f) startX -= worldW;
        for (float x = startX; x < static_cast<float>(screenW);
             x += worldW) {
            if (night < 1.0f) {
                DrawTextureEx(assets.bgDay, {x, layout.offset.y}, 0.0f,
                              layout.scale,
                              {255, 255, 255, alpha(1.0f - night)});
            }
            if (night > 0.0f) {
                DrawTextureEx(assets.bgNight, {x, layout.offset.y}, 0.0f,
                              layout.scale,
                              {255, 255, 255, alpha(night)});
            }
        }
    }

    if (baseW > 0.0f && baseH > 0.0f) {
        const float baseY = layout.offset.y +
                            (static_cast<float>(cfg::WORLD_H) - baseH) *
                                layout.scale;
        const float screenBaseW = baseW * layout.scale;
        float bx = layout.offset.x + std::fmod(g.groundScroll *
                                                   layout.scale,
                                               screenBaseW);
        while (bx > 0.0f) bx -= screenBaseW;
        for (; bx < static_cast<float>(screenW); bx += screenBaseW) {
            DrawTextureEx(assets.base, {bx, baseY}, 0.0f, layout.scale,
                          groundTint);
        }
    }

    // clip everything to the 288x512 world; otherwise off-screen pipes would
    // spill into the letterbox bars on wide windows
    const int viewX = static_cast<int>(std::floor(layout.offset.x));
    const int viewY = static_cast<int>(std::floor(layout.offset.y));
    const int viewW = static_cast<int>(
        std::ceil(static_cast<float>(cfg::WORLD_W) * layout.scale));
    const int viewH = static_cast<int>(
        std::ceil(static_cast<float>(cfg::WORLD_H) * layout.scale));

    // Slightly dim the side panels so the playfield stays the focus.
    const unsigned char barShade = alpha(30.0f + 90.0f * night);
    if (viewX > 0) {
        DrawRectangle(0, 0, viewX, screenH, {0, 0, 0, barShade});
    }
    if (viewX + viewW < screenW) {
        DrawRectangle(viewX + viewW, 0, screenW - (viewX + viewW),
                      screenH, {0, 0, 0, barShade});
    }
    if (viewY > 0) {
        DrawRectangle(0, 0, screenW, viewY, {0, 0, 0, barShade});
    }
    if (viewY + viewH < screenH) {
        DrawRectangle(0, viewY + viewH, screenW,
                      screenH - (viewY + viewH), {0, 0, 0, barShade});
    }

    BeginScissorMode(viewX, viewY, viewW, viewH);
    BeginMode2D(cam);

    // ----- background (cross-fade the two art layers) -----
    if (night < 1.0f) {
        DrawTexture(assets.bgDay, 0, 0,
                    {255, 255, 255, alpha(1.0f - night)});
    }
    if (night > 0.0f) {
        DrawTexture(assets.bgNight, 0, 0,
                    {255, 255, 255, alpha(night)});
        DrawRectangleGradientV(
            0, 0, cfg::WORLD_W, static_cast<int>(PLAYABLE_H),
            {6, 12, 34, alpha(140.0f * night)},
            {18, 34, 68, alpha(50.0f * night)});
    } else {
        // Subtle horizon lift so the flat sky has some depth.
        DrawRectangleGradientV(0, 0, cfg::WORLD_W,
                               static_cast<int>(PLAYABLE_H),
                               {0, 0, 0, 0},
                               {255, 255, 255, 16});
    }

    daynight::DrawScenery(g.daynightSt, night);

    // ----- pipes -----
    if (night < 1.0f) {
        const Color tint = {255, 255, 255, alpha(1.0f - night)};
        for (const auto& p : g.pipes) {
            if (!p.spawned) continue;
            DrawPipe(assets.pipeBody, p.x, 0.0f, p.topH, true, tint);
            DrawPipe(assets.pipeBody, p.x, p.bottomY,
                     PLAYABLE_H - p.bottomY, false, tint);
        }
    }
    if (night > 0.0f) {
        const Color tint = {255, 255, 255, alpha(night)};
        for (const auto& p : g.pipes) {
            if (!p.spawned) continue;
            DrawPipe(assets.pipeBodyNight, p.x, 0.0f, p.topH, true, tint);
            DrawPipe(assets.pipeBodyNight, p.x, p.bottomY,
                     PLAYABLE_H - p.bottomY, false, tint);
        }
    }

    // ----- ground -----
    Rectangle bsrc = {0, 0, baseW, baseH};
    if (baseW > 0.0f && baseH > 0.0f) {
        float gx = g.groundScroll;
        while (gx < static_cast<float>(cfg::WORLD_W)) {
            Rectangle bdst = {gx, static_cast<float>(cfg::WORLD_H) - baseH,
                              baseW, baseH};
            DrawTexturePro(assets.base, bsrc, bdst, {0, 0}, 0.0f, groundTint);
            gx += baseW;
        }
    }
    DrawLine(0, cfg::WORLD_H - cfg::phys::GROUND_H, cfg::WORLD_W,
             cfg::WORLD_H - cfg::phys::GROUND_H, cfg::vis::GROUND_LINE);

    // ----- bird (dest is the rotation centre, not the top-left) -----
    Texture2D& birdTex = [&]() -> Texture2D& {
        const int frame = static_cast<int>(g.bird.flapAnim * 3.0f) % 3;
        if (frame == 0) return assets.birdUp;
        if (frame == 1) return assets.birdMid;
        return assets.birdDown;
    }();
    Rectangle birdSrc = {0, 0, static_cast<float>(birdTex.width),
                         static_cast<float>(birdTex.height)};
    Rectangle birdDst = {g.bird.pos.x, g.bird.pos.y,
                         static_cast<float>(birdTex.width),
                         static_cast<float>(birdTex.height)};
    Vector2 birdOrigin = {birdDst.width * 0.5f, birdDst.height * 0.5f};
    DrawTexturePro(birdTex, birdSrc, birdDst, birdOrigin,
                   g.bird.rotation, WHITE);

    if (g.debug) {
        DrawRectangleLinesEx(BirdRect(g.bird), 1.0f, RED);
        for (const auto& p : g.pipes) {
            if (p.spawned) DrawPipeHitboxes(p);
        }
    }

    fx::gFx.DrawParticles();
    fx::gFx.DrawFlash();

    // ----- HUD -----
    if (g.state == PLAYING) {
        ui::DrawHud(g.score, g.highScore);
        if (g.daynightSt.justStartedFade) {
            const float toast = 1.0f - std::fabs(night * 2.0f - 1.0f);
            ui::DrawNightfallToast(cfg::WORLD_W * 0.5f, 92.0f, toast);
        }
    }

    // ----- overlays -----
    if (g.state == START) {
        ui::DrawStartScreen(g.highScore);
    } else if (g.state == GAMEOVER) {
        LayoutGameOverButtons();
        ui::DrawGameOver(g.score, g.highScore, g.isNewBest);
        ui::DrawButton(gRestartButton);
        ui::DrawButton(gMenuButton);
    } else if (g.state == PAUSED) {
        LayoutPauseButtons();
        ui::DrawPause(g.score);
        ui::DrawButton(gResumeButton);
        ui::DrawButton(gRestartButton);
        ui::DrawButton(gMenuButton);
    }

    EndMode2D();
    EndScissorMode();

    // A thin frame makes the playfield read as a deliberate framed view even
    // when the extended background fills the entire window.
    DrawRectangleLinesEx({static_cast<float>(viewX) - 1.0f,
                          static_cast<float>(viewY) - 1.0f,
                          static_cast<float>(viewW) + 2.0f,
                          static_cast<float>(viewH) + 2.0f},
                         2.0f, {0, 0, 0, 95});
    DrawRectangleLinesEx({static_cast<float>(viewX),
                          static_cast<float>(viewY),
                          static_cast<float>(viewW),
                          static_cast<float>(viewH)},
                         1.0f, {255, 255, 255, 32});
}

} // namespace game
