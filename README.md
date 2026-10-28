# Flappy Bird (C++ / raylib)

A polished Flappy Bird clone in C++17 using [raylib 6.2](https://www.raylib.com/)
(vendored under `third_party/raylib`). Cross-platform: builds on **Windows** and
**Linux** from a single CMake project.

## Features

- **Gameplay:** classic flap-through-pipes action with a fixed 120 Hz timestep
  so it feels identical on 60 / 144 / variable-refresh displays.
- **Better graphics:** supersampled 2× render target (576×1024) letterboxed
  into a resizable window, nearest-filtered pixel art, parallax background,
  scrolling ground, bird rotation / squash-stretch, pipe shading, plus
  particles (dust, score sparkles, death explosion), screen shake and flash.
- **High score:** persisted between runs (`%APPDATA%\FlappyBird` on Windows,
  `~/.local/share/flappy-bird` on Linux) with atomic writes and a "NEW BEST"
  ribbon.
- **Pause / play:** `P` or `Esc` toggles a dimmed pause overlay with
  Resume / Restart / Menu; auto-pauses when the window loses focus.
- **Day → night:** reaching **20 points** starts a smooth ~2.2 s transition to
  night (red pipes, stars, moon, fireflies). Stays night until the run ends,
  then resets to day.
- **Audio:** all provided `.wav` sounds (wing, point, swoosh, hit, die) with
  pitch variation on flaps; `M` toggles mute.
- **Window icon:** `assets/favicon.ico` is embedded into the Windows `.exe`
  (via `resources/flappy.rc`) and applied at runtime on Linux (X11/XWayland).

## Controls

| Key | Action |
|-----|--------|
| Space / Up / Click | Flap (also start) |
| P / Esc | Pause / resume |
| R | Restart |
| M | Mute / unmute |
| F11 / Alt+Enter | Fullscreen / windowed |

## Build

### Linux / macOS
```bash
./build.sh
./build/flappybird
```

### Windows
```bat
build.bat
build\flappybird.exe
```

Requires CMake ≥ 3.22 and a C++17 compiler. raylib is vendored, so no
external game-library installation is needed.

### Project layout
```
CMakeLists.txt          # build (Windows + Linux)
build.sh / build.bat    # build wrappers
README.md
assets/                 # sprites + audio + favicon.ico
resources/flappy.rc     # Windows .exe icon + version info
src/
  main.cpp              # window, icon, fixed-timestep loop
  config.{hpp,cpp}      # world size, tuning constants
  view.{hpp,cpp}        # letterboxed world→window scaling
  game.{hpp,cpp}        # bird, pipes, collisions, states
  ui.{hpp,cpp}          # HUD, menus, buttons, medals, digit score
  fx.{hpp,cpp}          # particles, shake, flash
  daynight.{hpp,cpp}    # day→night transition & night scenery
  assets.{hpp,cpp}      # asset loading & path resolution
  icon.{hpp,cpp}        # ICO parser for the runtime window icon
  storage.{hpp,cpp}     # persisted high score
third_party/raylib/     # vendored raylib 6.2 source
```

## Notes

- On native Wayland (not XWayland) the window title-bar icon is ignored by the
  compositor — this is a Wayland limitation, not a bug. Windows and X11/XWayland
  show the favicon correctly.
- The Windows build is produced from the same source; it is not compiled on
  this Linux host (no MSVC/MinGW available here), but all Windows-specific
  code is standard `.rc` + CMake.
- `assets/` is copied next to the binary after building so the game runs
  straight from `build/`.
