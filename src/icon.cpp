#include "icon.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <raylib.h>

namespace {

#pragma pack(push, 1)
struct IcoHeader {
    uint16_t reserved;
    uint16_t type;      // 1 = icon, 2 = cursor
    uint16_t count;
};

struct IcoEntry {
    uint8_t  width;     // 0 = 256
    uint8_t  height;    // 0 = 256
    uint8_t  colorCount;
    uint8_t  reserved;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t bytesInRes;
    uint32_t imageOffset;
};

struct BmpHeader {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
};
#pragma pack(pop)

// Read a little-endian value safely from a byte buffer.
template <typename T>
T ReadLE(const unsigned char* p) {
    T v = 0;
    for (size_t i = 0; i < sizeof(T); ++i) v |= static_cast<T>(p[i]) << (8 * i);
    return v;
}

bool LoadFileBytes(const char* path, std::vector<unsigned char>& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size <= 0) { std::fclose(f); return false; }
    out.resize(static_cast<size_t>(size));
    if (std::fread(out.data(), 1, out.size(), f) != out.size()) {
        std::fclose(f);
        return false;
    }
    std::fclose(f);
    return true;
}

// Pick the best icon frame: highest bit depth, then largest dimensions.
int PickEntry(const IcoHeader& header, const IcoEntry* entries) {
    int best = -1;
    int bestKey = -1;
    for (uint16_t i = 0; i < header.count; ++i) {
        const IcoEntry& e = entries[i];
        int w = e.width == 0 ? 256 : static_cast<int>(e.width);
        int h = e.height == 0 ? 256 : static_cast<int>(e.height);
        int key = (static_cast<int>(e.bitCount) * 100000) + (w * 1000) + h;
        if (key > bestKey) {
            bestKey = key;
            best = static_cast<int>(i);
        }
    }
    return best;
}

} // namespace

Image LoadWindowIconFromIco(const char* icoPath) {
    Image empty = {nullptr, 0, 0, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    if (!icoPath) return empty;
    std::vector<unsigned char> data;
    if (!LoadFileBytes(icoPath, data)) return empty;
    if (data.size() < sizeof(IcoHeader)) return empty;

    const IcoHeader* hdr = reinterpret_cast<const IcoHeader*>(data.data());
    if (hdr->reserved != 0 || hdr->type != 1 || hdr->count == 0) return empty;

    const size_t headerSize = sizeof(IcoHeader);
    const size_t entrySize = sizeof(IcoEntry);
    if (data.size() < headerSize + hdr->count * entrySize) return empty;

    const IcoEntry* entries =
        reinterpret_cast<const IcoEntry*>(data.data() + headerSize);
    int idx = PickEntry(*hdr, entries);
    if (idx < 0) return empty;

    const IcoEntry& e = entries[idx];
    uint32_t offset = e.imageOffset;
    if (offset + sizeof(BmpHeader) > data.size()) return empty;

    const BmpHeader* bmp =
        reinterpret_cast<const BmpHeader*>(data.data() + offset);
    if (bmp->size < 40) return empty;
    if (bmp->bitCount != 32) return empty;
    if (bmp->compression != 0 && bmp->compression != 3) return empty; // RGB or bitfields

    int w = bmp->width;
    // In an ICO, the DIB height covers the XOR image plus the AND mask, so
    // the actual image height is half of it. Ignoring this makes the buffer
    // bounds check fail for every normal .ico file.
    int h = bmp->height / 2;
    if (w <= 0 || h <= 0) return empty;

    // Pixel data starts after the DIB header (+ color masks for bitfields).
    size_t pixelOffset = offset + bmp->size;
    if (bmp->compression == 3) pixelOffset += 12; // RGB masks
    if (pixelOffset + static_cast<size_t>(w) * h * 4 > data.size()) return empty;

    const unsigned char* src = data.data() + pixelOffset;

    // ICO pixels are BGRA, bottom-up. Convert to RGBA, top-down for raylib.
    unsigned char* rgba =
        static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(w * h * 4)));
    if (!rgba) return empty;

    for (int y = 0; y < h; ++y) {
        const unsigned char* srcRow =
            src + static_cast<size_t>((h - 1 - y)) * static_cast<size_t>(w) * 4;
        unsigned char* dstRow = rgba + static_cast<size_t>(y) * static_cast<size_t>(w) * 4;
        for (int x = 0; x < w; ++x) {
            // src: B G R A  ->  dst: R G B A
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2];
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1];
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0];
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3];
        }
    }

    Image img;
    img.data = rgba;
    img.width = w;
    img.height = h;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return img;
}

void UnloadWindowIcon(Image image) {
    if (image.data) MemFree(image.data);
    image.data = nullptr;
    image.width = 0;
    image.height = 0;
}
