#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "platform/platform.hpp"

namespace platform {

std::string ExecutableDir() {
    char path[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return ".";

    std::string result(path, n);
    const std::size_t slash = result.find_last_of("\\/");
    if (slash != std::string::npos) {
        result.resize(slash);
    } else {
        result = ".";
    }
    return result;
}

std::string UserDataDir() {
    if (const char* appdata = std::getenv("APPDATA")) {
        return std::string(appdata) + "\\FlappyBird";
    }
    return ".";
}

std::string HighScorePath() {
    return UserDataDir() + "\\highscore.txt";
}

std::vector<std::string> SystemAssetDirs() {
    return {};
}

void ApplyWindowIcon(const std::string&) {}

void ShowError(const char* title, const char* message) {
    MessageBoxA(nullptr, message, title, MB_OK | MB_ICONERROR);
}

} // namespace platform
