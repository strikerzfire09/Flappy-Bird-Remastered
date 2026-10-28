#include "config.hpp"

#include <string>

#include "assets.hpp"
#include "game.hpp"
#include "icon.hpp"
#include "ui.hpp"

#include <raylib.h>

namespace {

void SetWindowIconFromFile() {
#if !defined(_WIN32)
    Image icon = LoadWindowIconFromIco("assets/favicon.ico");
    if (icon.data) {
        SetWindowIcon(icon);
        // raylib/GLFW copies icon pixel data, so we intentionally do not free
        // icon.data here (one-time startup allocation, ~64 KB, leak is OK).
        return;
    }
    Image icon2 = LoadWindowIconFromIco(
        (std::string(FLAPPYBIRD_ASSETS_DIR) + "/favicon.ico").c_str());
    if (icon2.data) {
        SetWindowIcon(icon2);
        return;
    }
#else
    (void)0;
#endif
}

void ApplyWindowMode(bool fullscreen, int windowW, int windowH) {
    if (fullscreen) {
        ToggleBorderlessWindowed();
    } else {
        if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) || IsWindowFullscreen()) {
            ToggleBorderlessWindowed();
        }
        SetWindowSize(windowW, windowH);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    cfg::Runtime rt;
    (void)argc;
    (void)argv;

    // Fixed window sized to match the world aspect ratio (288:512 = 0.5625).
    // The view module letterboxes when the window is resized.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(cfg::WINDOW_W, cfg::WINDOW_H, cfg::WINDOW_TITLE);
    SetWindowMinSize(cfg::WORLD_W / 2, cfg::WORLD_H / 2);

    SetWindowIconFromFile();

    // Audio (guard: may fail if no device).
    if (!IsAudioDeviceReady()) InitAudioDevice();
    SetMasterVolume(cfg::audio::VOLUME_MASTER);

    if (!assets.LoadAll()) {
        TraceLog(LOG_WARNING,
                 "Some assets failed to load. Check that 'assets/' is next to "
                 "the executable or in the source tree.");
    }
    ui::Init();
    game::GameInit();

    SetTargetFPS(cfg::TARGET_FPS);

    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        // --- input (once per rendered frame) ---
        bool flap = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                    IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);

        if (IsKeyPressed(KEY_M)) {
            rt.mute = !rt.mute;
            SetMasterVolume(rt.mute ? 0.0f : cfg::audio::VOLUME_MASTER);
        }

        if (IsKeyPressed(KEY_F11) ||
            (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            rt.fullscreen = !rt.fullscreen;
            ApplyWindowMode(rt.fullscreen, rt.windowW, rt.windowH);
        }

        if (IsKeyPressed(KEY_F3)) rt.showFps = !rt.showFps;
        if (IsKeyPressed(KEY_F1)) rt.debug = !rt.debug;

        game::gGame.flapPressed = flap || game::gGame.flapPressed;

        if (IsKeyPressed(KEY_P)) game::gGame.pausePressed = true;
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (game::gGame.state == game::GAMEOVER)
                game::gGame.menuPressed = true;
            else
                game::gGame.pausePressed = true;
        }
        if (IsKeyPressed(KEY_R) &&
            (game::gGame.state == game::GAMEOVER ||
             game::gGame.state == game::PAUSED)) {
            game::gGame.restartPressed = true;
        }

        // Auto-pause when the window loses focus mid-run.
        if (!IsWindowFocused() && game::gGame.state == game::PLAYING) {
            game::gGame.state = game::PAUSED;
        }

        // --- fixed-timestep update ---
        accumulator += GetFrameTime();
        int steps = 0;
        while (accumulator >= cfg::FIXED_DT && steps < 5) {
            game::GameFixedUpdate();
            accumulator -= cfg::FIXED_DT;
            ++steps;
        }
        if (steps == 5) accumulator = 0.0;

        // --- render ---
        BeginDrawing();
        game::GameRender();

        if (rt.showFps) DrawFPS(10, 10);
        if (rt.debug) {
            const char* dbg =
                TextFormat("FPS: %d  Score: %d  State: %d", GetFPS(),
                           game::gGame.score,
                           static_cast<int>(game::gGame.state));
            DrawText(dbg, 10, 30, 10, WHITE);
        }
        EndDrawing();
    }

    ui::Shutdown();
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
    return 0;
}
