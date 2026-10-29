#ifndef FLAPPYBIRD_ASSETS_HPP
#define FLAPPYBIRD_ASSETS_HPP

#include <raylib.h>
#include <string>
#include <vector>

// Central asset loader. Locates the assets folder (next to the executable,
// current working directory, compile-time source dir, or install prefix),
// loads every texture/sound once with nearest-neighbor filtering for crisp
// pixel art, and unloads everything on shutdown.
class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    bool LoadAll();            // returns false if any required asset is missing
    void UnloadAll();          // safe to call more than once
    std::string ResolveAsset(const char* relativePath) const;

    // ----- textures -----
    Texture2D bgDay;
    Texture2D bgNight;
    Texture2D base;
    Texture2D birdUp;
    Texture2D birdMid;
    Texture2D birdDown;
    Texture2D pipeBody;   // green (day)
    Texture2D pipeBodyNight; // red (night)
    Texture2D digit[10];
    Texture2D gameOver;
    Texture2D message;

    // ----- sounds -----
    Sound sndWing;
    Sound sndPoint;
    Sound sndSwoosh;
    Sound sndHit;
    Sound sndDie;

    bool ready = false;

private:
    std::string Resolve(const char* relativePath) const;
    bool LoadTextureEx(const char* relativePath, Texture2D& out);
    Sound LoadSoundEx(const char* relativePath);
    std::string baseDir;
    std::vector<std::string> roots;
};

extern AssetManager assets;

#endif // FLAPPYBIRD_ASSETS_HPP
