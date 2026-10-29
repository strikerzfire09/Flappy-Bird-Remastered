#include "storage.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "platform/platform.hpp"

#include <raylib.h>

namespace {

int ParseInt(const char* text, int fallback) {
    if (!text) return fallback;
    char* end = nullptr;
    long v = std::strtol(text, &end, 10);
    if (end == text || v < 0) return fallback;
    return static_cast<int>(v);
}

} // namespace

namespace storage {

int LoadHighScore() {
    std::string path = platform::HighScorePath();
    if (!FileExists(path.c_str())) return 0;
    char* text = LoadFileText(path.c_str());
    int value = 0;
    if (text) {
        value = ParseInt(text, 0);
        UnloadFileText(text);
    }
    if (value < 0) value = 0;
    return value;
}

bool SaveHighScore(int score) {
    std::string dir = platform::UserDataDir();
    if (!DirectoryExists(dir.c_str())) {
        if (MakeDirectory(dir.c_str()) != 0) return false;
    }

    std::string path = platform::HighScorePath();
    std::string tmp = path + ".tmp";

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d\n", score);

    if (!SaveFileText(tmp.c_str(), buf)) return false;
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        // Fall back: write directly.
        return SaveFileText(path.c_str(), buf);
    }
    return true;
}

const char* StoragePath() {
    static std::string path = platform::HighScorePath();
    return path.c_str();
}

} // namespace storage
