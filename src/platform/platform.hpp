#ifndef FLAPPYBIRD_PLATFORM_HPP
#define FLAPPYBIRD_PLATFORM_HPP

#include <string>
#include <vector>

namespace platform {

// Directory containing the running executable, without a trailing separator.
std::string ExecutableDir();

// Per-user directory for this game's saved data, without a trailing separator.
std::string UserDataDir();

// Full path to the persisted high-score file.
std::string HighScorePath();

// Additional read-only asset roots for this platform (may be empty).
std::vector<std::string> SystemAssetDirs();

// Apply the runtime window icon where the platform supports it. On Windows
// the icon is embedded in the executable, so this is a no-op.
void ApplyWindowIcon(const std::string& icoPath);

// Show a fatal startup error using the platform's native mechanism.
void ShowError(const char* title, const char* message);

} // namespace platform

#endif // FLAPPYBIRD_PLATFORM_HPP
