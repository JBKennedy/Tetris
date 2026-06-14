#ifndef HIGHSCORE_H_INCLUDED
#define HIGHSCORE_H_INCLUDED

#include <string>
#include <vector>

// Maximum number of entries kept in the leaderboard
inline constexpr int MAX_HIGH_SCORES{ 10 };

// Maximum characters a player name can be
inline constexpr int MAX_NAME_LENGTH{ 12 };

struct HighScore
{
    std::string name;
    int score{ 0 };
    int level{ 1 };
};

// Returns the directory (with trailing slash) where save files should be written.
// Desktop: returns "./"
// Android/iOS: returns platform-provided app data path (implement this per-platform later)
std::string getPersistentDataPath();

// Load up to MAX_HIGH_SCORES entries from disk.
// Returns an empty vector if the file doesn't exist yet (first run).
std::vector<HighScore> loadHighScores();

// Write the leaderboard to disk, sorted highest-first, capped at MAX_HIGH_SCORES.
void saveHighScores(std::vector<HighScore> scores);

// Returns true if 'score' is good enough to appear in the leaderboard
// (either the board isn't full yet, or it beats the lowest entry).
bool isHighScore(int score, const std::vector<HighScore>& leaderboard);

// Insert a new entry and re-sort / trim to MAX_HIGH_SCORES. Does not save to disk.
void insertHighScore(HighScore entry, std::vector<HighScore>& leaderboard);

#endif // HIGHSCORE_H_INCLUDED
