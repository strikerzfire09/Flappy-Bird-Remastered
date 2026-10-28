#ifndef FLAPPYBIRD_STORAGE_HPP
#define FLAPPYBIRD_STORAGE_HPP

#include "config.hpp"

// Persistent high score. Reads/writes a small text file in a per-platform
// data directory so the best score survives between runs.
namespace storage {

// Load the persisted high score (0 if none / unreadable).
int LoadHighScore();

// Save the high score atomically. Returns true on success.
bool SaveHighScore(int score);

// Path used for storage (debug aid).
const char* StoragePath();

} // namespace storage

#endif // FLAPPYBIRD_STORAGE_HPP
