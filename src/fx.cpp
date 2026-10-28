#include "fx.hpp"

#include <algorithm>
#include <cmath>

#include <raylib.h>
#include <rlgl.h>

namespace fx {

Effects gFx;

namespace {

float RandF(float a, float b) {
    constexpr int kSteps = 10000;
    float t = static_cast<float>(GetRandomValue(0, kSteps)) /
              static_cast<float>(kSteps);
    return a + (b - a) * t;
}

Color WithAlpha(Color c, float a) {
    c.a = static_cast<unsigned char>(c.a * a);
    return c;
}

Particle* FreeParticle() {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!gFx.particles[i].active) return &gFx.particles[i];
    }
    return nullptr;
}

void Emit(Vector2 at, Vector2 vel, float life, float size, Color color) {
    Particle* p = FreeParticle();
    if (!p) return;
    p->active = true;
    p->pos = at;
    p->vel = vel;
    p->life = p->lifeMax = life;
    p->size = size;
    p->color = color;
}

} // namespace

void Effects::Update(float dt) {
    if (shake > 0.0f) {
        shake -= cfg::vis::SHAKE_DECAY * dt;
        if (shake < 0.0f) shake = 0.0f;
    }
    if (flash > 0.0f) {
        flash -= cfg::vis::FLASH_DECAY * dt;
        if (flash < 0.0f) flash = 0.0f;
    }

    const float drag = std::pow(cfg::vis::PARTICLE_DRAG, dt * 60.0f);
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        Particle& p = particles[i];
        if (!p.active) continue;

        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += cfg::phys::GRAVITY * 0.35f * dt;
        p.vel.x *= drag;
        p.vel.y *= drag;
        p.life -= dt;
        if (p.life <= 0.0f) p.active = false;
    }
}

void Effects::Reset() {
    for (auto& p : particles) p.active = false;
    shake = 0.0f;
    flash = 0.0f;
}

void Effects::DrawShakeOffset(Vector2& out) const {
    if (shake <= 0.0f) {
        out = {0.0f, 0.0f};
        return;
    }
    out = {RandF(-shake, shake), RandF(-shake, shake)};
}

void Effects::DrawFlash() {
    if (flash <= 0.0f) return;
    Color c = {255, 255, 255,
               static_cast<unsigned char>(220.0f * flash)};
    // Oversized so camera shake can never expose the world edges.
    DrawRectangle(-32, -32, cfg::WORLD_W + 64, cfg::WORLD_H + 64, c);
}

void Effects::DrawParticles() {
    bool any = false;
    for (const auto& p : particles) {
        if (p.active) { any = true; break; }
    }
    if (!any) return;

    // One batch for every particle instead of one draw call per particle.
    rlSetTexture(0);
    rlBegin(RL_QUADS);
    for (const auto& p : particles) {
        if (!p.active) continue;
        float lifePct = (p.lifeMax > 0.0f) ? (p.life / p.lifeMax) : 0.0f;
        if (lifePct > 1.0f) lifePct = 1.0f;
        Color c = WithAlpha(p.color, lifePct);
        rlColor4ub(c.r, c.g, c.b, c.a);

        float x = std::floor(p.pos.x);
        float y = std::floor(p.pos.y);
        float s = std::max(1.0f, std::floor(p.size));
        rlVertex2f(x, y);
        rlVertex2f(x, y + s);
        rlVertex2f(x + s, y + s);
        rlVertex2f(x + s, y);
    }
    rlEnd();
    rlSetTexture(0);
}

void SpawnDust(Vector2 at, int count) {
    for (int i = 0; i < count; ++i) {
        Emit({at.x + RandF(-4.0f, 4.0f), at.y + RandF(-4.0f, 4.0f)},
             {RandF(-32.0f, 32.0f), RandF(20.0f, 75.0f)},
             RandF(0.14f, 0.34f), RandF(1.5f, 3.0f),
             {196, 184, 148, 210});
    }
}

void SpawnScoreSparkle(Vector2 at) {
    for (int i = 0; i < 7; ++i) {
        float ang = RandF(0.0f, 6.2831853f);
        float speed = RandF(35.0f, 110.0f);
        Emit({at.x + RandF(-7.0f, 7.0f), at.y + RandF(-7.0f, 7.0f)},
             {std::cos(ang) * speed, std::sin(ang) * speed},
             RandF(0.28f, 0.55f), RandF(1.5f, 3.0f),
             {255, 232, 128, 255});
    }
}

void SpawnDeathExplosion(Vector2 at) {
    for (int i = 0; i < 32; ++i) {
        float ang = RandF(0.0f, 6.2831853f);
        float speed = RandF(55.0f, 210.0f);
        Color c = (i % 3 == 0) ? Color{224, 74, 68, 255}
                               : Color{236, 214, 160, 255};
        Emit({at.x + RandF(-5.0f, 5.0f), at.y + RandF(-5.0f, 5.0f)},
             {std::cos(ang) * speed, std::sin(ang) * speed - 30.0f},
             RandF(0.35f, 0.9f), RandF(1.5f, 4.0f), c);
    }
}

void TriggerShake(float magnitude) {
    if (magnitude > gFx.shake) gFx.shake = magnitude;
}

void TriggerFlash(float alpha) {
    if (alpha > gFx.flash) gFx.flash = alpha;
}

} // namespace fx
