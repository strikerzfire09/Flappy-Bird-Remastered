#include "storage.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <raylib.h>

namespace {

std::string AppDataDir() {
#if defined(_WIN32)
    if (const char* appdata = std::getenv("APPDATA")) {
        std::string dir = appdata;
        dir += "\\FlappyBird";
        return dir;
    }
    // Fallback to the executable's folder.
    return ".";
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        std::string dir = home;
        dir += "/Library/Application Support/FlappyBird";
        return dir;
    }
    return ".";
#else
    if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
        std::string dir = xdg;
        dir += "/flappy-bird";
        return dir;
    }
    if (const char* home = std::getenv("HOME")) {
        std::string dir = home;
        dir += "/.local/share/flappy-bird";
        return dir;
    }
    return ".";
#endif
}

std::string HighScorePath() {
    std::string dir = AppDataDir();
#if defined(_WIN32)
    dir += "\\highscore.txt";
#else
    dir += "/highscore.txt";
#endif
    return dir;
}

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
    std::string path = HighScorePath();
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
    std::string dir = AppDataDir();
    if (!DirectoryExists(dir.c_str())) {
        if (MakeDirectory(dir.c_str()) != 0) return false;
    }

    std::string path = HighScorePath();
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
    static std::string path = HighScorePath();
    return path.c_str();
}

} // namespace storage
