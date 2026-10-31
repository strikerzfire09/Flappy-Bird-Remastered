#include "assets.hpp"

#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "embedded_assets.hpp"
#include "platform/platform.hpp"
#include <raylib.h>

AssetManager assets;

namespace {

bool FileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");

    if (f) {
        std::fclose(f);
        return true;
    }

    return false;
}

// Ordered list of candidate asset roots.
std::vector<std::string> CandidateRoots(
    const std::string& exeDir,
    const std::string& compileDir
) {
    std::vector<std::string> roots;

    roots.push_back(exeDir + "/assets");
    roots.push_back(exeDir + "/../share/flappybird/assets");
    roots.push_back("./assets");
    roots.push_back(compileDir);

    for (const auto& dir : platform::SystemAssetDirs()) {
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

    roots = CandidateRoots(
        platform::ExecutableDir(),
        baseDir
    );
}

AssetManager::~AssetManager() {
    UnloadAll();
}

std::string AssetManager::Resolve(const char* relativePath) const {

    for (const auto& root : roots) {

        std::string candidate = root + "/" + relativePath;

        if (FileExists(candidate)) {
            return candidate;
        }

    }

    // Best-guess path for useful error messages.
    return baseDir + "/" + relativePath;
}

std::string AssetManager::ResolveAsset(const char* relativePath) const {
    return Resolve(relativePath);
}

// ---------------------------------------------------------------- textures

bool AssetManager::LoadTextureEx(
    const char* relativePath,
    Texture2D& out
) {

    // First attempt: load the embedded asset.
    if (const auto* asset = embedded_assets::Find(relativePath)) {

        if (asset->size > static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            )) {
            out = {};
            return false;
        }

        std::string path = relativePath;

        const std::size_t dot = path.find_last_of('.');

        if (dot == std::string::npos) {
            out = {};
            return false;
        }

        const std::string extension = path.substr(dot);

        Image img = LoadImageFromMemory(
            extension.c_str(),
            asset->data,
            static_cast<int>(asset->size)
        );

        if (img.data == nullptr) {
            out = {};
            return false;
        }

        out = LoadTextureFromImage(img);

        // Keep pixel art crisp.
        if (out.id != 0) {
            SetTextureFilter(out, TEXTURE_FILTER_POINT);
        }

        UnloadImage(img);

        return out.id != 0;
    }

    // Second attempt: filesystem fallback for development.
    std::string path = Resolve(relativePath);

    Image img = LoadImage(path.c_str());

    if (img.data == nullptr) {
        out = {};
        return false;
    }

    out = LoadTextureFromImage(img);

    if (out.id != 0) {
        SetTextureFilter(out, TEXTURE_FILTER_POINT);
    }

    UnloadImage(img);

    return out.id != 0;
}

// ---------------------------------------------------------------- audio

Sound AssetManager::LoadSoundEx(const char* relativePath) {

    // Never attempt audio loading without an initialized audio device.
    if (!IsAudioDeviceReady()) {
        return {};
    }

    std::string base = relativePath;

    const std::size_t dot = base.find_last_of('.');

    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }

    // First attempt: embedded WAV, then embedded OGG.
    for (const std::string& extension : {".wav", ".ogg"}) {

        const std::string embeddedPath = base + extension;

        const auto* asset = embedded_assets::Find(embeddedPath);

        if (!asset) {
            continue;
        }

        if (asset->size > static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            )) {
            continue;
        }

        Wave wave = LoadWaveFromMemory(
            extension.c_str(),
            asset->data,
            static_cast<int>(asset->size)
        );

        if (wave.data == nullptr) {
            continue;
        }

        Sound sound = LoadSoundFromWave(wave);

        UnloadWave(wave);

        if (sound.stream.buffer != nullptr) {
            return sound;
        }
    }

    // Second attempt: filesystem fallback for development.
    const std::string wav = base + ".wav";
    const std::string ogg = base + ".ogg";

    std::string path = Resolve(wav.c_str());

    if (!FileExists(path)) {
        path = Resolve(ogg.c_str());
    }

    if (!FileExists(path)) {
        return {};
    }

    return LoadSound(path.c_str());
}

// ---------------------------------------------------------------- load all

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

        std::snprintf(
            name,
            sizeof(name),
            "sprites/%d.png",
            i
        );

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

// ---------------------------------------------------------------- unload

void AssetManager::UnloadAll() {

    auto texture = [](Texture2D& t) {

        if (t.id != 0) {
            UnloadTexture(t);
        }

        t = {};
    };

    auto sound = [](Sound& s) {

        if (s.stream.buffer != nullptr) {
            UnloadSound(s);
        }

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

    for (auto& d : digit) {
        texture(d);
    }

    texture(gameOver);
    texture(message);

    ready = false;
}