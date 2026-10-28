#ifndef FLAPPYBIRD_DAYNIGHT_HPP
#define FLAPPYBIRD_DAYNIGHT_HPP

#include "config.hpp"

namespace daynight {

enum Phase { DAY, FADING_TO_NIGHT, NIGHT };

// Day -> night state. Reaching the score threshold starts a one-way
// transition; night stays until the run ends.
struct State {
    Phase phase = DAY;
    float t = 0.0f;          // 0..1 transition progress
    bool triggered = false;  // threshold reached this run?
    bool justStartedFade = false;
    float anim = 0.0f;       // scenery animation clock (seconds)
};

// Advance the transition and scenery animation. Call once per fixed step.
void Update(State& st, int score, float dt);

// Eased (smoothstep) night blend, 0 = day, 1 = night.
float Ease(const State& st);

bool IsNight(const State& st);

// Draw clouds, sun/moon, stars and fireflies. Call after the background and
// before pipes. `night` is Ease(st).
void DrawScenery(const State& st, float night);

// Clear-color / pipe tint blended by the current day→night ease.
Color SkyColor(const State& st);
Color PipeTint(const State& st);

} // namespace daynight

#endif // FLAPPYBIRD_DAYNIGHT_HPP
