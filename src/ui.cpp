#include "ui.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "assets.hpp"
#include "config.hpp"

namespace ui {

namespace {

constexpr int LABEL_PAD = 2;
constexpr int FONT_SMALL = 10;
constexpr int FONT_LARGE = 20;

struct Label {
    std::string text;
    int size = 0;
    unsigned int fgKey = 0;
    unsigned int outlineKey = 0;
    Texture2D texture = {};
    int width = 0;
    int height = 0;
};

std::vector<Label> gLabels;

unsigned int ColorKey(Color c) {
    return (static_cast<unsigned int>(c.r) << 24) |
           (static_cast<unsigned int>(c.g) << 16) |
           (static_cast<unsigned int>(c.b) << 8) |
           static_cast<unsigned int>(c.a);
}

void DrawTextShadowed(const char* text, int x, int y, int fontSize,
                      Color fg, Color shadow) {
    DrawText(text, x + 1, y + 2, fontSize, shadow);
    DrawText(text, x, y, fontSize, fg);
}

const Label& GetLabel(const char* text, int size, Color fg, Color outline) {
    const unsigned int fgKey = ColorKey(fg);
    const unsigned int outlineKey = ColorKey(outline);

    for (const auto& label : gLabels) {
        if (label.size == size && label.fgKey == fgKey &&
            label.outlineKey == outlineKey && label.text == text) {
            return label;
        }
    }

    Label label;
    label.text = text;
    label.size = size;
    label.fgKey = fgKey;
    label.outlineKey = outlineKey;

    Vector2 measured = MeasureTextEx(GetFontDefault(), text,
                                     static_cast<float>(size), 1.0f);
    label.width = static_cast<int>(std::ceil(measured.x));
    label.height = static_cast<int>(std::ceil(measured.y));

    const int texW = label.width + LABEL_PAD * 2;
    const int texH = label.height + LABEL_PAD * 2;
    if (texW <= 0 || texH <= 0) {
        label.width = 0;
        label.height = 0;
        gLabels.push_back(label);
        return gLabels.back();
    }

    Image image = GenImageColor(texW, texH, BLANK);
    Font font = GetFontDefault();

    // Soft drop shadow, 1px outline, then the fill.
    ImageDrawTextEx(&image, font, text,
                    {static_cast<float>(LABEL_PAD + 1),
                     static_cast<float>(LABEL_PAD + 2)},
                    static_cast<float>(size), 1.0f, {0, 0, 0, 120});
    const int ox[4] = {-1, 1, 0, 0};
    const int oy[4] = {0, 0, -1, 1};
    for (int i = 0; i < 4; ++i) {
        ImageDrawTextEx(&image, font, text,
                        {static_cast<float>(LABEL_PAD + ox[i]),
                         static_cast<float>(LABEL_PAD + oy[i])},
                        static_cast<float>(size), 1.0f, outline);
    }
    ImageDrawTextEx(&image, font, text,
                    {static_cast<float>(LABEL_PAD),
                     static_cast<float>(LABEL_PAD)},
                    static_cast<float>(size), 1.0f, fg);

    label.texture = LoadTextureFromImage(image);
    SetTextureFilter(label.texture, TEXTURE_FILTER_POINT);
    UnloadImage(image);

    gLabels.push_back(label);
    return gLabels.back();
}

void DrawLabel(const char* text, int x, int y, int size, Color fg,
               Color outline) {
    const Label& label = GetLabel(text, size, fg, outline);
    if (label.texture.id == 0) return;
    DrawTexture(label.texture, x - LABEL_PAD, y - LABEL_PAD, WHITE);
}

void DrawLabelCentered(const char* text, int centerX, int y, int size,
                       Color fg, Color outline) {
    const Label& label = GetLabel(text, size, fg, outline);
    if (label.texture.id == 0) return;
    DrawTexture(label.texture, centerX - label.width / 2 - LABEL_PAD,
                y - LABEL_PAD, WHITE);
}

void DrawPanel(const Rectangle& r) {
    Rectangle shadow = {r.x + 2, r.y + 4, r.width, r.height};
    DrawRectangleRounded(shadow, 0.08f, 6, {0, 0, 0, 130});
    DrawRectangleRounded(r, 0.08f, 6, cfg::vis::UI_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.08f, 6, 2.0f,
                                {214, 226, 242, 220});
    Rectangle inner = {r.x + 3, r.y + 3, r.width - 6, r.height - 6};
    DrawRectangleRoundedLinesEx(inner, 0.08f, 6, 1.0f,
                                {92, 108, 138, 120});
    DrawRectangle(static_cast<int>(r.x + 12), static_cast<int>(r.y + 2),
                  static_cast<int>(r.width - 24), 1, {255, 255, 255, 45});
}

void DrawMedal(int score, int x, int y) {
    const char* name = nullptr;
    Color ring = WHITE;
    Color fill = {100, 100, 110, 255};

    if (score >= cfg::feat::MEDAL_PLATINUM) {
        name = "PLATINUM";
        ring = {228, 232, 244, 255};
        fill = {170, 176, 196, 255};
    } else if (score >= cfg::feat::MEDAL_GOLD) {
        name = "GOLD";
        ring = {255, 220, 92, 255};
        fill = {222, 174, 44, 255};
    } else if (score >= cfg::feat::MEDAL_SILVER) {
        name = "SILVER";
        ring = {222, 228, 238, 255};
        fill = {156, 166, 182, 255};
    } else if (score >= cfg::feat::MEDAL_BRONZE) {
        name = "BRONZE";
        ring = {232, 158, 96, 255};
        fill = {170, 102, 54, 255};
    } else {
        return;
    }

    constexpr float radius = 20.0f;
    DrawCircle(x, y + 2, static_cast<int>(radius + 3), {0, 0, 0, 90});
    DrawCircle(x, y, static_cast<int>(radius + 2),
               {ring.r, ring.g, ring.b, 55});
    DrawCircle(x, y, static_cast<int>(radius), fill);
    DrawCircleLines(x, y, static_cast<int>(radius), ring);
    DrawCircleLines(x, y, static_cast<int>(radius - 3),
                    {ring.r, ring.g, ring.b, 150});
    DrawCircle(static_cast<int>(x - radius * 0.34f),
               static_cast<int>(y - radius * 0.34f),
               static_cast<int>(radius * 0.22f), {255, 255, 255, 95});

    const char initial[2] = {name[0], '\0'};
    DrawLabelCentered(initial, x, y - FONT_SMALL / 2 - 1, FONT_SMALL,
                      {255, 255, 255, 255}, {30, 24, 12, 255});
    DrawLabelCentered(name, x, y + 27, FONT_SMALL,
                      {232, 238, 248, 255}, cfg::vis::TEXT_OUTLINE_DARK);
}

} // namespace

void Init() {
    // Pre-warm every static label used by the menus.
    const Color white = cfg::vis::TEXT_WHITE;
    const Color darkOutline = cfg::vis::TEXT_OUTLINE_DARK;
    const Color gold = {255, 226, 120, 255};

    GetLabel("START", FONT_LARGE, white, darkOutline);
    GetLabel("RESTART", FONT_LARGE, white, darkOutline);
    GetLabel("MAIN MENU", FONT_LARGE, white, darkOutline);
    GetLabel("RESUME", FONT_LARGE, white, darkOutline);
    GetLabel("PAUSED", FONT_LARGE, white, darkOutline);
    GetLabel("TAP / SPACE / CLICK TO FLY", FONT_SMALL, cfg::vis::TEXT_DARK,
             cfg::vis::TEXT_OUTLINE_LIGHT);
    GetLabel("BEST", FONT_SMALL, white, darkOutline);
    GetLabel("NEW BEST!", FONT_SMALL, gold, darkOutline);
    GetLabel("NIGHTFALL", FONT_SMALL, {236, 240, 255, 255}, darkOutline);
    GetLabel("BRONZE", FONT_SMALL, white, darkOutline);
    GetLabel("SILVER", FONT_SMALL, white, darkOutline);
    GetLabel("GOLD", FONT_SMALL, white, darkOutline);
    GetLabel("PLATINUM", FONT_SMALL, white, darkOutline);
    GetLabel("BEST 0", FONT_SMALL, white, darkOutline);
}

void Shutdown() {
    for (auto& label : gLabels) {
        if (label.texture.id != 0) UnloadTexture(label.texture);
    }
    gLabels.clear();
}

void UpdateButton(Button& btn, Vector2 worldMouse) {
    btn.hovered = CheckCollisionPointRec(worldMouse, btn.bounds);
    btn.clicked = btn.hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        btn.armed = btn.hovered;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        btn.pressed = btn.armed && btn.hovered;
        btn.armed = false;
    } else {
        btn.pressed = false;
        if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) btn.armed = false;
    }
}

bool ButtonPressed(Button& btn) {
    if (!btn.pressed) return false;
    btn.pressed = false;
    return true;
}

void DrawButton(const Button& btn) {
    Color face = btn.clicked ? Color{22, 30, 48, 248}
                 : btn.hovered ? Color{64, 112, 190, 248}
                               : Color{40, 52, 78, 246};
    Color border = btn.hovered ? Color{255, 255, 255, 255}
                               : Color{202, 216, 236, 235};

    Rectangle shadow = {btn.bounds.x + 2, btn.bounds.y + 3, btn.bounds.width,
                        btn.bounds.height};
    DrawRectangleRounded(shadow, 0.28f, 6, {0, 0, 0, 140});
    DrawRectangleRounded(btn.bounds, 0.28f, 6, face);
    DrawRectangleRoundedLinesEx(btn.bounds, 0.28f, 6, 2.0f, border);

    Rectangle inner = {btn.bounds.x + 3, btn.bounds.y + 3,
                       btn.bounds.width - 6, btn.bounds.height - 6};
    DrawRectangleRoundedLinesEx(inner, 0.28f, 6, 1.0f,
                                {255, 255, 255, 34});

    const Label& label = GetLabel(btn.label, FONT_LARGE,
                                  cfg::vis::TEXT_WHITE,
                                  cfg::vis::TEXT_OUTLINE_DARK);
    if (label.texture.id == 0) return;
    int x = static_cast<int>(btn.bounds.x +
                             (btn.bounds.width - label.width) * 0.5f);
    int y = static_cast<int>(btn.bounds.y +
                             (btn.bounds.height - label.height) * 0.5f) - 1;
    DrawTexture(label.texture, x - LABEL_PAD, y - LABEL_PAD, WHITE);
}

void DrawScoreDigits(int score, int centerX, int y, float scale) {
    if (score < 0) score = 0;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", score);

    float totalW = 0.0f;
    for (const char* p = buf; *p; ++p) {
        totalW += static_cast<float>(assets.digit[*p - '0'].width) * scale;
    }

    // Drop shadow.
    float sx = static_cast<float>(centerX) - totalW * 0.5f + 2.0f * scale;
    for (const char* p = buf; *p; ++p) {
        Texture2D& t = assets.digit[*p - '0'];
        Rectangle src = {0, 0, static_cast<float>(t.width),
                         static_cast<float>(t.height)};
        Rectangle dst = {sx, static_cast<float>(y) + 2.0f * scale,
                         static_cast<float>(t.width) * scale,
                         static_cast<float>(t.height) * scale};
        DrawTexturePro(t, src, dst, {0, 0}, 0.0f, {0, 0, 0, 130});
        sx += dst.width;
    }

    float x = static_cast<float>(centerX) - totalW * 0.5f;
    for (const char* p = buf; *p; ++p) {
        Texture2D& t = assets.digit[*p - '0'];
        Rectangle src = {0, 0, static_cast<float>(t.width),
                         static_cast<float>(t.height)};
        Rectangle dst = {x, static_cast<float>(y),
                         static_cast<float>(t.width) * scale,
                         static_cast<float>(t.height) * scale};
        DrawTexturePro(t, src, dst, {0, 0}, 0.0f, WHITE);
        x += dst.width;
    }
}

void DrawHud(int score, int highScore) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "BEST %d", highScore);
    DrawLabelCentered(buf, cfg::WORLD_W / 2, 6, FONT_SMALL,
                      cfg::vis::TEXT_WHITE, cfg::vis::TEXT_OUTLINE_DARK);

    DrawScoreDigits(score, cfg::WORLD_W / 2, 20, 1.0f);
}

void DrawStartScreen(int highScore) {
    Texture2D& msg = assets.message;
    const float scale = 0.78f;
    Rectangle src = {0, 0, static_cast<float>(msg.width),
                     static_cast<float>(msg.height)};
    Rectangle dst = {cfg::WORLD_W / 2 - msg.width * scale * 0.5f, 48.0f,
                     static_cast<float>(msg.width) * scale,
                     static_cast<float>(msg.height) * scale};
    DrawTexturePro(msg, src, dst, {0, 0}, 0.0f, WHITE);

    float blink = 0.5f + 0.5f * std::sinf(GetTime() * 4.0f);
    if (blink > 0.25f) {
        DrawLabelCentered("TAP / SPACE / CLICK TO FLY", cfg::WORLD_W / 2,
                          330, FONT_SMALL, cfg::vis::TEXT_DARK,
                          cfg::vis::TEXT_OUTLINE_LIGHT);
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "BEST %d", highScore);
    DrawLabelCentered(buf, cfg::WORLD_W / 2, 352, FONT_SMALL,
                      cfg::vis::TEXT_DARK, cfg::vis::TEXT_OUTLINE_LIGHT);
}

void DrawGameOver(int score, int highScore, bool newBest) {
    DrawRectangle(0, 0, cfg::WORLD_W, cfg::WORLD_H, {0, 0, 0, 120});

    Texture2D& go = assets.gameOver;
    const float scale = 0.9f;
    Rectangle src = {0, 0, static_cast<float>(go.width),
                     static_cast<float>(go.height)};
    Rectangle dst = {cfg::WORLD_W / 2 - go.width * scale * 0.5f, 54.0f,
                     static_cast<float>(go.width) * scale,
                     static_cast<float>(go.height) * scale};
    DrawTexturePro(go, src, dst, {0, 0}, 0.0f, WHITE);

    DrawPanel({54.0f, 118.0f, 180.0f, 178.0f});

    DrawScoreDigits(score, cfg::WORLD_W / 2, 136, 0.9f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "BEST %d", highScore);
    DrawLabelCentered(buf, cfg::WORLD_W / 2, 182, FONT_SMALL,
                      {214, 222, 236, 255}, cfg::vis::TEXT_OUTLINE_DARK);

    DrawMedal(score, cfg::WORLD_W / 2, 234);

    if (newBest) {
        DrawNewBestRibbon(cfg::WORLD_W / 2, 102);
    }
}

void DrawPause(int score) {
    DrawRectangle(0, 0, cfg::WORLD_W, cfg::WORLD_H, {0, 0, 0, 150});
    DrawPanel({49.0f, 156.0f, 190.0f, 194.0f});

    DrawLabelCentered("PAUSED", cfg::WORLD_W / 2, 174, FONT_LARGE,
                      cfg::vis::TEXT_WHITE, cfg::vis::TEXT_OUTLINE_DARK);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "SCORE %d", score);
    DrawLabelCentered(buf, cfg::WORLD_W / 2, 202, FONT_SMALL,
                      {208, 218, 234, 255}, cfg::vis::TEXT_OUTLINE_DARK);
}

void DrawNewBestRibbon(int centerX, int y) {
    const Label& label = GetLabel("NEW BEST!", FONT_SMALL,
                                  {255, 226, 120, 255},
                                  cfg::vis::TEXT_OUTLINE_DARK);
    const int w = label.width + 18;
    const int h = label.height + 8;
    Rectangle r = {static_cast<float>(centerX - w / 2),
                   static_cast<float>(y - h / 2), static_cast<float>(w),
                   static_cast<float>(h)};
    DrawRectangleRounded({r.x + 1, r.y + 2, r.width, r.height}, 0.35f, 5,
                         {0, 0, 0, 120});
    DrawRectangleRounded(r, 0.35f, 5, {82, 66, 20, 240});
    DrawRectangleRoundedLinesEx(r, 0.35f, 5, 1.0f, {255, 218, 96, 255});
    DrawTexture(label.texture, centerX - label.width / 2 - LABEL_PAD,
                y - label.height / 2 - LABEL_PAD, WHITE);
}

void DrawNightfallToast(float centerX, float y, float alpha) {
    if (alpha <= 0.01f) return;
    const Label& label = GetLabel("NIGHTFALL", FONT_SMALL,
                                  {236, 240, 255, 255},
                                  cfg::vis::TEXT_OUTLINE_DARK);
    if (label.texture.id == 0) return;

    unsigned char a = static_cast<unsigned char>(
        std::fmin(255.0f, std::fmax(0.0f, 255.0f * alpha)));
    const int w = label.width + 20;
    const int h = label.height + 10;
    Rectangle r = {centerX - w / 2.0f, y - h / 2.0f,
                   static_cast<float>(w), static_cast<float>(h)};
    DrawRectangleRounded(r, 0.35f, 5, {18, 24, 48,
                                       static_cast<unsigned char>(220 * alpha)});
    DrawRectangleRoundedLinesEx(r, 0.35f, 5, 1.0f,
                                {174, 190, 232, a});
    DrawTexture(label.texture, static_cast<int>(centerX - label.width / 2.0f) -
                                   LABEL_PAD,
                static_cast<int>(y - label.height / 2.0f) - LABEL_PAD,
                {255, 255, 255, a});
}

} // namespace ui
