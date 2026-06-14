#include "HighScore.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <filesystem>

// -----------------------------------------------------------------------
// Platform path resolution
// Desktop implementation - just use the working directory.
// Replace or #ifdef this function per platform when porting to mobile.
// -----------------------------------------------------------------------
std::string getPersistentDataPath()
{
#if defined(_WIN32)
    const char* appdata{ std::getenv("APPDATA") };
    if (appdata)
    {
        std::filesystem::path p{ appdata };
        p /= "Tetris";
        std::filesystem::create_directories(p);
        return p.string() + "/";
    }
    return "./";

#elif defined(__APPLE__)
    const char* home{ std::getenv("HOME") };
    if (home)
    {
        std::filesystem::path p{ home };
        p /= "Library/Application Support/Tetris";
        std::filesystem::create_directories(p);
        return p.string() + "/";
    }
    return "./";

#else
    // Linux: XDG or fallback to current dir
    const char* xdg{ std::getenv("XDG_DATA_HOME") };
    if (xdg)
    {
        std::filesystem::path p{ xdg };
        p /= "Tetris";
        std::filesystem::create_directories(p);
        return p.string() + "/";
    }
    const char* home{ std::getenv("HOME") };
    if (home)
    {
        std::filesystem::path p{ home };
        p /= ".local/share/Tetris";
        std::filesystem::create_directories(p);
        return p.string() + "/";
    }
    return "./";
#endif
}

// -----------------------------------------------------------------------
// File format (plain text, one entry per line):
//   <score> <level> <name with possible spaces>
// Example:
//   15400 3 Hermione
//   8200 2 Harry Potter
// Score and level come first os parsing doesn't need to handle
// names-with-spaces specially - just read two ints, then getline the rest.
// -----------------------------------------------------------------------
static const std::string SAVE_FILENAME{ "highscores.text" };

std::vector<HighScore> loadHighScores()
{
    std::vector<HighScore> scores;

    std::ifstream file(getPersistentDataPath() + SAVE_FILENAME);
    if (!file.is_open())
    {
        // Normal on first run - no scores yet
        return scores;
    }

    HighScore entry;
    std::string name;
    while (file >> entry.score >> entry.level)
    {
        file.ignore(1);           // Consume the space between level and name,
        std::getline(file, name); // then read rest of line
        entry.name = name;
        scores.emplace_back(entry);

        if (static_cast<int>(scores.size()) >= MAX_HIGH_SCORES)
            break;
    }

    return scores;
}

void saveHighScores(std::vector<HighScore> scores)
{
    // Sort descending by score before writing
    std::sort(scores.begin(), scores.end(),
              [](const HighScore& a, const HighScore& b){ return a.score > b.score; });

    // Trim to max
    if (static_cast<int>(scores.size()) > MAX_HIGH_SCORES)
        scores.resize(MAX_HIGH_SCORES);

    std::ofstream file(getPersistentDataPath() + SAVE_FILENAME, std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "Warning: could not write high scores to disk.\n";
        return;
    }

    for (const auto& entry : scores)
    {
        file << entry.score << ' ' << entry.level << ' ' << entry.name << '\n';
    }
}

bool isHighScore(int score, const std::vector<HighScore>& leaderboard)
{
    if (static_cast<int>(leaderboard.size()) < MAX_HIGH_SCORES)
        return true; // board not full yet - any score qualifies

    // Board is full - only beats it if score > lowest entry
    // (leaderboard is kept sorted highest-first, so lowest is at back)
    return score > leaderboard.back().score;
}

void insertHighScore(HighScore entry, std::vector<HighScore>& leaderboard)
{
    leaderboard.emplace_back(entry);
    std::sort(leaderboard.begin(), leaderboard.end(),
              [](const HighScore& a, const HighScore& b){ return a.score > b.score; });

    if (static_cast<int>(leaderboard.size()) > MAX_HIGH_SCORES)
        leaderboard.resize(MAX_HIGH_SCORES);
}
