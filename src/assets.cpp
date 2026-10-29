#include "assets.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "platform/platform.hpp"

#include <raylib.h>

AssetManager assets;

namespace {

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
    for (const auto& dir : platform::SystemAssetDirs()) {  // OS install paths
        roots.push_back(dir);
    }
    return roots;
}

} // namespace

AssetManager::AssetManager() {
#if defined(FLAPPYBIRD_ASSETS_DIR)
    baseDir = FLAPPYBIRD_ASSETS_DIR;
#else
    baseDir = "assets";
#endif
    roots = CandidateRoots(platform::ExecutableDir(), baseDir);
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

std::string AssetManager::ResolveAsset(const char* relativePath) const {
    return Resolve(relativePath);
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
    // Loading audio with no initialized device can divide by a zero sample
    // rate inside miniaudio; stay silent instead of crashing.
    if (!IsAudioDeviceReady()) return {};

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
    // Guard every unload and clear the handles. This makes shutdown safe even
    // though the global destructor calls UnloadAll() again after main.
    auto texture = [](Texture2D& t) {
        if (t.id != 0) UnloadTexture(t);
        t = {};
    };
    auto sound = [](Sound& s) {
        if (s.stream.buffer != nullptr) UnloadSound(s);
        s = {};
    };

    sound(sndWing);
    sound(sndPoint);
    sound(sndSwoosh);
    sound(sndHit);
    sound(sndDie);

    texture(bgDay);
    texture(bgNight);
    texture(base);
    texture(birdUp);
    texture(birdMid);
    texture(birdDown);
    texture(pipeBody);
    texture(pipeBodyNight);
    for (auto& d : digit) texture(d);
    texture(gameOver);
    texture(message);

    ready = false;
}
