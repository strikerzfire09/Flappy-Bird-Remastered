# Flappy Bird Remastered

A native C++17 remake of Flappy Bird built on [raylib 6.2](https://www.raylib.com/).
The game simulates a fixed **288 × 512** world at **120 Hz**, renders crisp
pixel art into a resizable letterboxed window, and adds parallax scenery,
particles, screen shake, medals, and a day/night transition.

## Highlights

- **Fixed-timestep gameplay** — physics runs at 120 Hz, so the bird and pipes
  behave consistently on 60, 144, or variable-refresh displays.
- **Crisp presentation** — the logical 288 × 512 world is scaled with
  nearest-neighbor filtering and letterboxed at any window size. The default
  window is 432 × 768.
- **Atmosphere** — parallax background, animated bird, shaded pipes, dust and
  score particles, screen shake and flash, plus a full day/night transition
  that starts at **20 points** and brings stars, a moon, fireflies, and red
  pipes.
- **Progression** — persistent high score, "NEW BEST" feedback, and bronze,
  silver, gold, and platinum medals.
- **Comfort features** — pause menu, auto-pause when the window loses focus,
  restart, mute, and fullscreen.
- **Native packaging** — embedded Windows icon/version resource, runtime icon
  on Linux, static MSVC CRT, and a fully static MinGW runtime by default.
- **Separate platform layers** — Windows and Linux system code live in
  independent folders and build into independent output trees.

## Controls

| Key | Action |
| --- | --- |
| `Space` / `Up` / left click | Flap (also starts a run) |
| `P` / `Esc` | Pause / resume |
| `R` | Restart |
| `M` | Mute / unmute |
| `F11` / `Alt+Enter` | Fullscreen / windowed |
| `F3` | Toggle debug overlay (FPS, state, score) |

## Requirements

- CMake **3.22+**
- A C++17 compiler
- **Windows:** Visual Studio 2022 with the Desktop C++ workload, or
  MinGW-w64. The batch wrapper uses whichever CMake generator is available.
- **Linux:** OpenGL, X11, and audio development packages. raylib is not
  vendored: CMake downloads raylib 6.2 automatically on the first configure,
  so no raylib installation is needed (network access is required then).
- **macOS:** Xcode command-line tools.

## Building

Both build wrappers are **clean builders**: when the current platform already
has a successful executable, the wrapper deletes that platform's build
directory and configures/compiles from scratch. This prevents an old executable
or a stale CMake cache from being used after source changes. A partial build
that never produced an executable is kept so its error output can be inspected.

### Linux

```bash
./build.sh
./build/linux/bin/flappybird
```

### Windows

```bat
build.bat
build\windows\bin\flappybird.exe
```

### macOS

```bash
./build.sh
./build/macos/bin/flappybird
```

### Manual CMake build

Linux:

```bash
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --parallel
```

Windows:

```bat
cmake -S . -B build/windows
cmake --build build/windows --config Release --parallel
```

The executable and a copy of `assets/` are placed in the build's `bin/`
directory, so the game can be run directly from there.

The first configure downloads raylib 6.2 into `third_party/.cache/` (ignored by
Git). For offline builds, point CMake at an existing raylib checkout instead:
`-DFLAPPYBIRD_RAYLIB_SOURCE_DIR=/path/to/raylib`.

### Build outputs

| Platform | Build directory | Executable |
| --- | --- | --- |
| Linux | `build/linux/` | `build/linux/bin/flappybird` |
| Windows | `build/windows/` | `build/windows/bin/flappybird.exe` |
| macOS | `build/macos/` | `build/macos/bin/flappybird` |

`build/` is ignored by Git. With the current layout, running a wrapper after a
successful build removes only the matching platform folder; the other
platform's build is left alone. If an old pre-split `build/bin/` executable is
detected, the legacy build root is removed instead.

## Project layout

```text
CMakeLists.txt            # shared build, platform selection, asset copying
build.sh                  # clean Linux/macOS build wrapper
build.bat                 # clean Windows build wrapper
assets/                   # sprites, audio, favicon.ico
resources/
  flappy.rc.in            # generated Windows icon + version resource
src/
  main.cpp                # window, startup checks, fixed-timestep loop
  config.hpp              # world size, physics constants, visual tuning
  view.{hpp,cpp}          # letterboxed world-to-window scaling
  game.{hpp,cpp}          # bird, pipes, collisions, game states
  ui.{hpp,cpp}            # HUD, menus, medals, score rendering
  fx.{hpp,cpp}            # particles, shake, flash, backdrop helpers
  daynight.{hpp,cpp}      # day/night transition and night scenery
  assets.{hpp,cpp}        # asset discovery, loading, and unloading
  icon.{hpp,cpp}          # runtime ICO parser used on Linux
  storage.{hpp,cpp}       # persistent high score
  platform/
    platform.hpp          # shared OS abstraction
    linux/
      platform_linux.cpp
    windows/
      platform_windows.cpp
    macos/
      platform_macos.cpp
third_party/               # optional local raylib checkout (not tracked)
```

CMake selects exactly one file from `src/platform/`. The Windows platform
translation unit under `src/` is the only game source that includes
`<windows.h>`; keeping it separate avoids name and macro collisions with raylib
(`Rectangle`, `CloseWindow`, `ShowCursor`, and `LoadImage`).

## Saved data

The high score is written atomically to:

| Platform | High-score file |
| --- | --- |
| Windows | `%APPDATA%\FlappyBird\highscore.txt` |
| Linux | `$XDG_DATA_HOME/flappy-bird/highscore.txt` (fallback: `~/.local/share/flappy-bird/highscore.txt`) |
| macOS | `~/Library/Application Support/FlappyBird/highscore.txt` |

## Startup diagnostics

- On Windows, window-creation and missing-asset failures display a native error
  dialog instead of closing silently.
- On Linux and macOS, the same failures are written to standard error.
- Windows MinGW builds link the runtime statically, so the executable does not
  require `libwinpthread-1.dll`, `libstdc++-6.dll`, or `libgcc_s_*.dll`.
- MSVC builds use the static CRT and do not require the Visual C++
  redistributable.

If the Windows game exits immediately, rerun `build.bat` before launching the
new executable. The wrapper removes the previous successful build so a stale
binary cannot be started by mistake.

## Notes

- On native Wayland, the compositor may ignore the window title-bar icon. This
  is a Wayland limitation; X11/XWayland displays the favicon correctly, and the
  Windows icon is embedded in the executable.
- The game uses raylib's GLFW backend. On Linux, XWayland is used when running
  under a Wayland desktop.
- The original Flappy Bird concept and artwork belong to their respective
  owners; raylib is distributed under its own zlib license.
