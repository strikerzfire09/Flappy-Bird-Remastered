#ifndef FLAPPYBIRD_FX_HPP
#define FLAPPYBIRD_FX_HPP

#include "config.hpp"

namespace fx {

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;      // seconds remaining
    float lifeMax;
    float size;
    Color color;
    bool active = false;
};

constexpr int MAX_PARTICLES = 200;

struct Effects {
    Particle particles[MAX_PARTICLES];
    float shake = 0.0f; // current shake magnitude, world units
    float flash = 0.0f; // 0..1 white flash alpha

    void Update(float dt);
    void Reset();
    void DrawShakeOffset(Vector2& out) const;
    void DrawFlash();
    void DrawParticles();
};

extern Effects gFx;

// Spawn helpers
void SpawnDust(Vector2 at, int count);
void SpawnScoreSparkle(Vector2 at);
void SpawnDeathExplosion(Vector2 at);
void TriggerShake(float magnitude);
void TriggerFlash(float alpha);

} // namespace fx

#endif // FLAPPYBIRD_FX_HPP
