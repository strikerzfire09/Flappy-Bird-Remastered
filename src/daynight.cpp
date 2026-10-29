#include "daynight.hpp"

#include <cmath>

#include <raylib.h>
#include <rlgl.h>

#include "assets.hpp"

namespace daynight {

namespace {

constexpr int STAR_COUNT = 46;
constexpr int FIREFLY_COUNT = 9;
constexpr int CLOUD_COUNT = 4;

struct Star {
    float x;
    float y;
    float brightness;
};

struct Firefly {
    float x;
    float y;
    float phase;
    float speed;
    float radius;
};

struct Cloud {
    float x;
    float y;
    float size;
    float speed;
};

Star gStars[STAR_COUNT];
Firefly gFireflies[FIREFLY_COUNT];
Cloud gClouds[CLOUD_COUNT];
bool gInitialized = false;

float RandF(float a, float b) {
    constexpr int kSteps = 10000;
    float t = static_cast<float>(GetRandomValue(0, kSteps)) /
              static_cast<float>(kSteps);
    return a + (b - a) * t;
}

void EnsureInit() {
    if (gInitialized) return;
    for (auto& s : gStars) {
        s.x = RandF(2.0f, cfg::WORLD_W - 2.0f);
        s.y = RandF(8.0f, 300.0f);
        s.brightness = RandF(0.45f, 1.0f);
    }
    for (auto& f : gFireflies) {
        f.x = RandF(8.0f, cfg::WORLD_W - 8.0f);
        f.y = RandF(220.0f, 385.0f);
        f.phase = RandF(0.0f, 6.2831853f);
        f.speed = RandF(0.5f, 1.3f);
        f.radius = RandF(10.0f, 34.0f);
    }
    for (int i = 0; i < CLOUD_COUNT; ++i) {
        gClouds[i].x = RandF(-20.0f, cfg::WORLD_W + 20.0f);
        gClouds[i].y = 42.0f + i * 34.0f + RandF(-8.0f, 8.0f);
        gClouds[i].size = RandF(0.65f, 1.25f);
        gClouds[i].speed = RandF(1.2f, 3.2f);
    }
    gInitialized = true;
}

float Smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

unsigned char LerpAlpha(unsigned char a, float factor) {
    return static_cast<unsigned char>(
        std::fmax(0.0f, std::fmin(255.0f, a * factor)));
}

void DrawCloud(float x, float y, float size, Color color) {
    DrawCircle(static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(11.0f * size), color);
    DrawCircle(static_cast<int>(x + 14.0f * size),
               static_cast<int>(y - 6.0f * size),
               static_cast<int>(14.0f * size), color);
    DrawCircle(static_cast<int>(x + 30.0f * size),
               static_cast<int>(y + 1.0f * size),
               static_cast<int>(10.0f * size), color);
    DrawRectangle(static_cast<int>(x - 6.0f * size),
                  static_cast<int>(y), static_cast<int>(36.0f * size),
                  static_cast<int>(12.0f * size), color);
}

} // namespace

void Update(State& st, int score, float dt) {
    st.anim += dt;

    for (auto& f : gFireflies) {
        f.phase += f.speed * dt;
        f.x += std::sin(f.phase * 0.7f) * 3.0f * dt;
        f.y += std::cos(f.phase * 1.1f) * 2.0f * dt;
        if (f.x < 4.0f) f.x = cfg::WORLD_W - 4.0f;
        if (f.x > cfg::WORLD_W - 4.0f) f.x = 4.0f;
        if (f.y < 215.0f) f.y = 385.0f;
        if (f.y > 385.0f) f.y = 215.0f;
    }

    if (st.phase == NIGHT) return;

    if (st.phase == DAY) {
        if (score >= cfg::feat::DAY_NIGHT_SCORE) {
            st.phase = FADING_TO_NIGHT;
            st.t = 0.0f;
            st.triggered = true;
            st.justStartedFade = true;
            PlaySound(assets.sndSwoosh);
        }
        return;
    }

    st.t += dt / cfg::vis::DAY_NIGHT_FADE_SEC;
    if (st.t >= 1.0f) {
        st.t = 1.0f;
        st.phase = NIGHT;
        st.justStartedFade = false;
    }
}

float Ease(const State& st) {
    if (st.phase == DAY) return 0.0f;
    if (st.phase == NIGHT) return 1.0f;
    return Smoothstep(st.t);
}

bool IsNight(const State& st) { return st.phase != DAY; }

void DrawScenery(const State& st, float night) {
    EnsureInit();
    if (night < 0.0f) night = 0.0f;
    if (night > 1.0f) night = 1.0f;

    // ----- soft clouds (visible by day, tinted by night) -----
    for (const auto& c : gClouds) {
        float x = c.x + std::fmod(st.anim * c.speed, 380.0f);
        if (x > cfg::WORLD_W + 40.0f) x -= 380.0f;
        Color col = {
            static_cast<unsigned char>(255 - 100 * night),
            static_cast<unsigned char>(255 - 78 * night),
            static_cast<unsigned char>(255 - 30 * night),
            static_cast<unsigned char>(105 - 45 * night),
        };
        DrawCloud(x, c.y, c.size, col);
    }

    // ----- sun by day -----
    if (night < 0.999f) {
        float a = 1.0f - night;
        int sx = 238;
        int sy = 72;
        DrawCircle(sx, sy, 34, {255, 240, 170, LerpAlpha(52, a)});
        DrawCircle(sx, sy, 24, {255, 240, 160, LerpAlpha(105, a)});
        DrawCircle(sx, sy, 17, {255, 247, 196, LerpAlpha(245, a)});
        DrawCircle(sx - 5, sy - 5, 5, {255, 255, 230, LerpAlpha(170, a)});
    }

    // ----- moon by night -----
    if (night > 0.001f) {
        int mx = 236;
        int my = 74;
        DrawCircle(mx, my, 31, {180, 200, 255, LerpAlpha(40, night)});
        DrawCircle(mx, my, 19, {232, 236, 210, LerpAlpha(235, night)});
        DrawCircle(mx - 6, my - 3, 4, {206, 210, 188, LerpAlpha(160, night)});
        DrawCircle(mx + 5, my + 6, 3, {206, 210, 188, LerpAlpha(140, night)});
    }

    // ----- stars (one batched draw) -----
    if (night > 0.001f) {
        rlSetTexture(0);
        rlBegin(RL_QUADS);
        for (int i = 0; i < STAR_COUNT; ++i) {
            const Star& s = gStars[i];
            float twinkle =
                0.55f + 0.45f * std::sin(st.anim * 2.2f + i * 1.7f);
            unsigned char a =
                LerpAlpha(static_cast<unsigned char>(235 * s.brightness),
                          night * twinkle);
            if (a < 8) continue;
            rlColor4ub(255, 250, 220, a);
            float size = (s.brightness > 0.82f) ? 2.0f : 1.0f;
            rlVertex2f(s.x, s.y);
            rlVertex2f(s.x, s.y + size);
            rlVertex2f(s.x + size, s.y + size);
            rlVertex2f(s.x + size, s.y);
        }
        rlEnd();
        rlSetTexture(0);
    }

    // ----- fireflies -----
    if (night > 0.001f) {
        for (const auto& f : gFireflies) {
            float glow = 0.55f + 0.45f * std::sin(f.phase * 2.0f);
            unsigned char a = LerpAlpha(210, night * glow);
            DrawCircle(static_cast<int>(f.x), static_cast<int>(f.y), 2,
                       {255, 238, 130, a});
            DrawCircle(static_cast<int>(f.x), static_cast<int>(f.y), 5,
                       {255, 230, 100, LerpAlpha(40, night * glow)});
        }
    }
}

} // namespace daynight
