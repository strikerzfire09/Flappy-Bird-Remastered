#ifndef FLAPPYBIRD_VIEW_HPP
#define FLAPPYBIRD_VIEW_HPP

#include <raylib.h>

// Maps the fixed 288x512 logical world onto the current window. The world is
// scaled uniformly and letterboxed so the aspect ratio is never distorted.
namespace view {

struct Layout {
    float scale = 1.0f;
    Vector2 offset = {0.0f, 0.0f};

    Camera2D Camera() const {
        Camera2D cam = {};
        cam.offset = offset;
        cam.target = {0.0f, 0.0f};
        cam.rotation = 0.0f;
        cam.zoom = scale;
        return cam;
    }

    Vector2 ToWorld(Vector2 screen) const {
        return { (screen.x - offset.x) / scale,
                 (screen.y - offset.y) / scale };
    }

    Vector2 ToScreen(Vector2 world) const {
        return { world.x * scale + offset.x,
                 world.y * scale + offset.y };
    }
};

// Recompute from the current framebuffer size. Safe to call every frame.
Layout Compute();

} // namespace view

#endif // FLAPPYBIRD_VIEW_HPP
