#include "view.hpp"

#include <cmath>

#include "config.hpp"

namespace view {

Layout Compute() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    float scaleX = static_cast<float>(w) / static_cast<float>(cfg::WORLD_W);
    float scaleY = static_cast<float>(h) / static_cast<float>(cfg::WORLD_H);
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    // Keep the default 1.5x look, but snap larger scales to half steps so
    // pixel sizes stay as even as possible on big windows/fullscreen.
    if (scale >= 1.5f) {
        scale = std::floor(scale * 2.0f) / 2.0f;
    }
    if (scale < 0.25f) scale = 0.25f;

    Layout layout;
    layout.scale = scale;
    layout.offset.x =
        std::floor((static_cast<float>(w) - cfg::WORLD_W * scale) * 0.5f);
    layout.offset.y =
        std::floor((static_cast<float>(h) - cfg::WORLD_H * scale) * 0.5f);
    return layout;
}

} // namespace view
