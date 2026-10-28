#ifndef FLAPPYBIRD_UI_HPP
#define FLAPPYBIRD_UI_HPP

#include "config.hpp"

namespace ui {

// Simple clickable button. A press only fires if it starts and ends inside,
// which prevents clicks leaking between game states.
struct Button {
    Rectangle bounds = {0, 0, 0, 0};
    const char* label = "";
    bool hovered = false;
    bool clicked = false;
    bool armed = false;
    bool pressed = false;
};

// Build the cached label atlas. Call after assets are loaded.
void Init();
void Shutdown();

// ---- buttons ----
void UpdateButton(Button& btn, Vector2 worldMouse);
void DrawButton(const Button& btn);
bool ButtonPressed(Button& btn); // true once on a completed click

// ---- HUD / screens ----
void DrawScoreDigits(int score, int centerX, int y, float scale = 1.0f);
void DrawHud(int score, int highScore);

void DrawStartScreen(int highScore);
void DrawGameOver(int score, int highScore, bool newBest);
void DrawPause(int score);

void DrawNewBestRibbon(int centerX, int y);
void DrawNightfallToast(float centerX, float y, float alpha);

} // namespace ui

#endif // FLAPPYBIRD_UI_HPP
