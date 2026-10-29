#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "icon.hpp"
#include "platform/platform.hpp"

#include <raylib.h>

namespace platform {

std::string ExecutableDir() {
    char path[4096];
    const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n <= 0) return ".";

    std::string result(path, static_cast<std::size_t>(n));
    const std::size_t slash = result.find_last_of('/');
    if (slash != std::string::npos) {
        result.resize(slash);
    } else {
        result = ".";
    }
    return result;
}

std::string UserDataDir() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
        return std::string(xdg) + "/flappy-bird";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::string(home) + "/.local/share/flappy-bird";
    }
    return ".";
}

std::string HighScorePath() {
    return UserDataDir() + "/highscore.txt";
}

std::vector<std::string> SystemAssetDirs() {
    return {"/usr/local/share/flappybird/assets",
            "/usr/share/flappybird/assets"};
}

void ApplyWindowIcon(const std::string& icoPath) {
    Image icon = LoadWindowIconFromIco(icoPath.c_str());
    if (icon.data != nullptr) {
        SetWindowIcon(icon);
        UnloadWindowIcon(icon);
    }
}

void ShowError(const char* title, const char* message) {
    std::fprintf(stderr, "%s: %s\n", title, message);
}

} // namespace platform
