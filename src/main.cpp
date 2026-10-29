#include "config.hpp"

#include <algorithm>
#include <string>

#include "assets.hpp"
#include "daynight.hpp"
#include "fx.hpp"
#include "game.hpp"
#include "platform/platform.hpp"
#include "storage.hpp"
#include "ui.hpp"
#include "view.hpp"

#include <raylib.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(cfg::WINDOW_W, cfg::WINDOW_H, cfg::WINDOW_TITLE);
    if (!IsWindowReady()) {
        platform::ShowError(
            "Flappy Bird",
            "Could not create the game window.\n\n"
            "Make sure your graphics driver supports OpenGL 3.3.");
        return 1;
    }

    SetWindowMinSize(cfg::WINDOW_W / 2, cfg::WINDOW_H / 2);

    // ESC is a game key (pause / menu), not an exit key.
    SetExitKey(KEY_NULL);

    platform::ApplyWindowIcon(assets.ResolveAsset("favicon.ico"));

    if (!IsAudioDeviceReady()) InitAudioDevice();
    SetMasterVolume(cfg::audio::VOLUME_MASTER);

    if (!assets.LoadAll()) {
        platform::ShowError(
            "Flappy Bird",
            "Could not load the game assets.\n\n"
            "Keep the 'assets' folder next to flappybird.exe and try again.");
        assets.UnloadAll();
        if (IsAudioDeviceReady()) CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    ui::Init();
    game::GameInit();

    SetTargetFPS(cfg::TARGET_FPS);

    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        game::Game& g = game::gGame;

        // ----- sample input edges exactly once per frame -----
        const bool flap = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                          IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                          IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
        g.flapPressed = g.flapPressed || flap;

        const bool pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
        g.pausePressed = g.pausePressed || pause;
        g.menuPressed = g.menuPressed || IsKeyPressed(KEY_ESCAPE);
        g.restartPressed = g.restartPressed || IsKeyPressed(KEY_R);

        if (IsKeyPressed(KEY_M)) {
            static bool muted = false;
            muted = !muted;
            SetMasterVolume(muted ? 0.0f : cfg::audio::VOLUME_MASTER);
        }

        if (IsKeyPressed(KEY_F11) ||
            (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleBorderlessWindowed();
        }

        if (IsKeyPressed(KEY_F3)) {
            g.debug = !g.debug;
        }

        // Auto-pause when the window loses focus so the player does not fly
        // into a pipe while alt-tabbed.
        if (g.state == game::PLAYING && !IsWindowFocused()) {
            g.state = game::PAUSED;
        }

        // ----- fixed timestep (persistent accumulator) -----
        accumulator += static_cast<double>(GetFrameTime());
        if (accumulator > 0.25) accumulator = 0.25; // avoid spiral of death
        int steps = 0;
        while (accumulator >= cfg::FIXED_DT && steps < 8) {
            game::GameFixedUpdate();
            accumulator -= cfg::FIXED_DT;
            ++steps;
        }

        // ----- UI hit-testing once per frame, in world coordinates -----
        const view::Layout layout = view::Compute();
        game::GameFrameInput(layout.ToWorld(GetMousePosition()));

        // ----- draw -----
        BeginDrawing();
        ClearBackground(BLACK);
        game::GameRender();

        if (g.debug) {
            DrawFPS(10, 10);
            const char* info = TextFormat("state %d  score %d  best %d",
                                          static_cast<int>(g.state), g.score,
                                          g.highScore);
            DrawText(info, 10, 32, 16, LIME);
            DrawText("F3 debug   P/ESC pause   R restart   F11 fullscreen",
                     10, GetScreenHeight() - 22, 16, LIME);
        }

        EndDrawing();
    }

    storage::SaveHighScore(std::max(game::gGame.highScore, game::gGame.score));
    ui::Shutdown();
    assets.UnloadAll();
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
    return 0;
}
