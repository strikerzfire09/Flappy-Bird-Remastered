#include <mach-o/dyld.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "platform/platform.hpp"

namespace platform {

std::string ExecutableDir() {
    char path[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return ".";

    std::string result(path);
    const std::size_t slash = result.find_last_of('/');
    if (slash != std::string::npos) {
        result.resize(slash);
    } else {
        result = ".";
    }
    return result;
}

std::string UserDataDir() {
    if (const char* home = std::getenv("HOME")) {
        return std::string(home) + "/Library/Application Support/FlappyBird";
    }
    return ".";
}

std::string HighScorePath() {
    return UserDataDir() + "/highscore.txt";
}

std::vector<std::string> SystemAssetDirs() {
    return {};
}

void ApplyWindowIcon(const std::string&) {}

void ShowError(const char* title, const char* message) {
    std::fprintf(stderr, "%s: %s\n", title, message);
}

} // namespace platform
