#include "assets.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <raylib.h>

// Avoid windows.h — it collides with raylib (Rectangle, LoadImage, CloseWindow).
#if defined(_WIN32)
extern "C" unsigned long __stdcall GetModuleFileNameA(void* module, char* filename,
                                                      unsigned long size);
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

AssetManager assets;

namespace {

std::string ExeDir() {
#if defined(_WIN32)
    char path[512];
    unsigned long n = GetModuleFileNameA(nullptr, path, sizeof(path) - 1);
    if (n == 0) return ".";
    path[n] = '\0';
    std::string p = path;
    size_t slash = p.find_last_of("\\/");
    if (slash != std::string::npos) p = p.substr(0, slash);
    return p;
#elif defined(__APPLE__)
    char path[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string p = path;
        size_t slash = p.find_last_of('/');
        if (slash != std::string::npos) p = p.substr(0, slash);
        return p;
    }
    return ".";
#else
    char path[4096];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n > 0) {
        path[n] = '\0';
        std::string p = path;
        size_t slash = p.find_last_of('/');
        if (slash != std::string::npos) p = p.substr(0, slash);
        return p;
    }
    return ".";
#endif
}

bool FileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f) { std::fclose(f); return true; }
    return false;
}

// Ordered list of candidate asset roots.
std::vector<std::string> CandidateRoots(const std::string& exeDir,
                                        const std::string& compileDir) {
    std::vector<std::string> roots;
    roots.push_back(exeDir + "/assets");                 // next to the executable
    roots.push_back(exeDir + "/../share/flappybird/assets"); // layout install
    roots.push_back("./assets");                          // current working dir
    roots.push_back(compileDir);                          // source tree (dev)
#if !defined(_WIN32)
    roots.push_back("/usr/local/share/flappybird/assets"); // system install
    roots.push_back("/usr/share/flappybird/assets");
#endif
    return roots;
}

} // namespace

AssetManager::AssetManager() {
#if defined(FLAPPYBIRD_ASSETS_DIR)
    baseDir = FLAPPYBIRD_ASSETS_DIR;
#else
    baseDir = "assets";
#endif
    roots = CandidateRoots(ExeDir(), baseDir);
}

AssetManager::~AssetManager() { UnloadAll(); }

std::string AssetManager::Resolve(const char* relativePath) const {
    for (const auto& root : roots) {
        std::string cand = root + "/" + relativePath;
        if (FileExists(cand)) return cand;
    }
    // Return a best-guess path so the error message is useful.
    return baseDir + "/" + relativePath;
}

bool AssetManager::LoadTextureEx(const char* relativePath, Texture2D& out) {
    std::string path = Resolve(relativePath);
    Image img = LoadImage(path.c_str());
    if (img.data == nullptr) {
        out = {};
        return false;
    }
    // Keep pixel art crisp: nearest-neighbor filtering, no mipmaps.
    out = LoadTextureFromImage(img);
    SetTextureFilter(out, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return true;
}

Sound AssetManager::LoadSoundEx(const char* relativePath) {
    // Prefer WAV (PCM), fall back to OGG.
    std::string wav = relativePath;
    std::string ogg = relativePath;
    size_t dot = wav.rfind('.');
    if (dot != std::string::npos) {
        wav = wav.substr(0, dot) + ".wav";
        ogg = ogg.substr(0, dot) + ".ogg";
    } else {
        wav += ".wav";
        ogg += ".ogg";
    }
    std::string path = Resolve(wav.c_str());
    if (!FileExists(path)) path = Resolve(ogg.c_str());
    return LoadSound(path.c_str());
}

bool AssetManager::LoadAll() {
    auto req = [&](const char* rel, Texture2D& out) -> bool {
        if (!LoadTextureEx(rel, out)) {
            TraceLog(LOG_WARNING, "Missing texture: %s", rel);
            return false;
        }
        return true;
    };

    bool ok = true;
    ok &= req("sprites/background-day.png", bgDay);
    ok &= req("sprites/background-night.png", bgNight);
    ok &= req("sprites/base.png", base);
    ok &= req("sprites/yellowbird-upflap.png", birdUp);
    ok &= req("sprites/yellowbird-midflap.png", birdMid);
    ok &= req("sprites/yellowbird-downflap.png", birdDown);
    ok &= req("sprites/pipe-green.png", pipeBody);
    ok &= req("sprites/pipe-red.png", pipeBodyNight);
    for (int i = 0; i < 10; ++i) {
        char name[64];
        std::snprintf(name, sizeof(name), "sprites/%d.png", i);
        ok &= req(name, digit[i]);
    }
    ok &= req("sprites/gameover.png", gameOver);
    ok &= req("sprites/message.png", message);

    sndWing = LoadSoundEx("audio/wing");
    sndPoint = LoadSoundEx("audio/point");
    sndSwoosh = LoadSoundEx("audio/swoosh");
    sndHit = LoadSoundEx("audio/hit");
    sndDie = LoadSoundEx("audio/die");

    ready = ok;
    return ok;
}

void AssetManager::UnloadAll() {
    UnloadSound(sndWing);
    UnloadSound(sndPoint);
    UnloadSound(sndSwoosh);
    UnloadSound(sndHit);
    UnloadSound(sndDie);
    UnloadTexture(bgDay);
    UnloadTexture(bgNight);
    UnloadTexture(base);
    UnloadTexture(birdUp);
    UnloadTexture(birdMid);
    UnloadTexture(birdDown);
    UnloadTexture(pipeBody);
    UnloadTexture(pipeBodyNight);
    for (int i = 0; i < 10; ++i) UnloadTexture(digit[i]);
    UnloadTexture(gameOver);
    UnloadTexture(message);
    ready = false;
}
