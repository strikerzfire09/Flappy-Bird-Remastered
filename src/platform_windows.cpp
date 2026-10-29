#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

std::string GetExecutableDirectory()
{
    char path[MAX_PATH];

    DWORD n = GetModuleFileNameA(
        nullptr,
        path,
        static_cast<DWORD>(sizeof(path) - 1)
    );

    if (n == 0)
        return ".";

    path[n] = '\0';

    std::string p = path;
    const size_t slash = p.find_last_of("\\/");

    if (slash != std::string::npos)
        p.resize(slash);

    return p;
}

#endif