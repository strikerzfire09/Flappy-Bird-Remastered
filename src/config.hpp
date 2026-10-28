#ifndef FLAPPYBIRD_CONFIG_HPP
#define FLAPPYBIRD_CONFIG_HPP

#include <raylib.h>

// ---------------------------------------------------------------------------
// Logical world. The game always simulates and draws at this resolution;
// the view module scales it to the window with letterboxing.
// ---------------------------------------------------------------------------
namespace cfg {

inline constexpr int WORLD_W = 288;
inline constexpr int WORLD_H = 512;

inline constexpr int WINDOW_W = 432;
inline constexpr int WINDOW_H = 768;
inline constexpr char const* WINDOW_TITLE = "Flappy Bird";

// Target frame rate. Physics runs at a fixed 120 Hz timestep.
inline constexpr int TARGET_FPS = 60;
inline constexpr double FIXED_DT = 1.0 / 120.0;

// ---------------------------------------------------------------------------
// Physics / gameplay tuning.
// ---------------------------------------------------------------------------
namespace phys {
inline constexpr float GRAVITY = 520.0f;        // px / s^2
inline constexpr float FLAP_VELOCITY = -175.0f; // px / s upward impulse
inline constexpr float MAX_FALL_SPEED = 420.0f;
inline constexpr float PIPE_SPEED_START = 140.0f;
inline constexpr float PIPE_SPEED_MAX = 260.0f;
inline constexpr float PIPE_SPEED_ACCEL = 3.5f;    // added per point scored
inline constexpr float PIPE_SPACING = 154.0f;      // world px between pipe edges
inline constexpr float PIPE_MIN_X_FROM_EDGE = PIPE_SPACING;
inline constexpr float PIPE_SPAWN_INTERVAL = 1.35f; // seconds between spawns
inline constexpr float PIPE_GAP_START = 110.0f;
inline constexpr float PIPE_GAP_MIN = 88.0f;
inline constexpr float PIPE_GAP_SHRINK_PER_POINT = 1.8f;
inline constexpr float BIRD_X = 70.0f;
inline constexpr float GROUND_H = 112.0f;
inline constexpr float BIRD_RADIUS = 10.0f;

// Forgiving bird collision box (sprite is 34x24).
inline constexpr float BIRD_HITBOX_W = 24.0f;
inline constexpr float BIRD_HITBOX_H = 16.0f;
} // namespace phys

// ---------------------------------------------------------------------------
// Difficulty / feature thresholds.
// ---------------------------------------------------------------------------
namespace feat {
inline constexpr int DAY_NIGHT_SCORE = 20; // score at which night begins
inline constexpr int MEDAL_BRONZE = 10;
inline constexpr int MEDAL_SILVER = 20;
inline constexpr int MEDAL_GOLD = 30;
inline constexpr int MEDAL_PLATINUM = 40;
} // namespace feat

// ---------------------------------------------------------------------------
// Audio.
// ---------------------------------------------------------------------------
namespace audio {
inline constexpr float VOLUME_MASTER = 0.7f;
inline constexpr float VOLUME_SFX = 0.9f;
inline constexpr float WING_PITCH_MIN = 0.85f;
inline constexpr float WING_PITCH_MAX = 1.2f;
} // namespace audio

// ---------------------------------------------------------------------------
// Visual polish.
// ---------------------------------------------------------------------------
namespace vis {
inline constexpr float PARTICLE_DRAG = 0.90f;
inline constexpr float SHAKE_DECAY = 9.0f;
inline constexpr float FLASH_DECAY = 12.0f;
inline constexpr float DAY_NIGHT_FADE_SEC = 2.0f;
inline constexpr Color GROUND_LINE = {140, 130, 70, 255};
inline constexpr Color UI_PANEL = {22, 24, 34, 238};
inline constexpr Color UI_PANEL_BORDER = {240, 240, 248, 255};
inline constexpr Color TEXT_WHITE = {250, 250, 250, 255};
inline constexpr Color TEXT_DARK = {24, 38, 62, 255};
inline constexpr Color TEXT_OUTLINE_LIGHT = {232, 242, 250, 255};
inline constexpr Color TEXT_OUTLINE_DARK = {16, 18, 26, 255};
} // namespace vis

// ---------------------------------------------------------------------------
// Runtime configuration (toggled by the user at runtime).
// ---------------------------------------------------------------------------
struct Runtime {
    bool mute = false;
    bool fullscreen = false;
    bool debug = false;
    bool showFps = false;
    int windowW = WINDOW_W;
    int windowH = WINDOW_H;
};

} // namespace cfg

#endif // FLAPPYBIRD_CONFIG_HPP
