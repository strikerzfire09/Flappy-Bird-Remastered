#ifndef FLAPPYBIRD_ICON_HPP
#define FLAPPYBIRD_ICON_HPP

#include <raylib.h>

// Parse a Windows .ico file at runtime and pick the best frame for the
// window title-bar / taskbar icon. Used on Linux (X11/XWayland) where the
// executable icon is not embedded; on Windows the icon is embedded via .rc.
Image LoadWindowIconFromIco(const char* icoPath);
void UnloadWindowIcon(Image image);

#endif // FLAPPYBIRD_ICON_HPP
