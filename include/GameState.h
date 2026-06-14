#ifndef GAMESTATE_H_INCLUDED
#define GAMESTATE_H_INCLUDED

#include "LayoutConstants.h"
#include "Color.h"
#include "Tetromino.h"
#include "RenderFrame.h" // also pulls in GamePhase and HighScore via RenderFrame.h
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <random>

class GameState
{
public:
    GameState();
    void update(GLFWwindow* window, float deltaTime);
    render_data::RenderFrame buildRenderFrame() const;
    void handleInput(GLFWwindow* window, float deltaTime);
    void appendNameChar(unsigned int codepoint);

    // Called by pause menu buttons (keyboard shortcuts or future mouse clicks)
    void startOrResume();   // "Play" / "Resume" button
    void quitToMenu();      // "Quit" button - resets board and returns to pause menu

private:
    // --- Board ---
    Color board[config::BOARD_HEIGHT][config::BOARD_WIDTH]{ Color::black };

    Tetromino currentPiece;
    Tetromino nextPiece;
    Tetromino holdPiece;
    Tetromino ghostPiece;

    // --- Game phase ---
    GamePhase m_phase{ GamePhase::Paused };
    bool      m_gameHasStarted{ false }; // false until first Play is pressed

    // --- Hold ---
    bool m_hasHold{ false };
    bool m_canHold{ true }; // resets to true each time a piece locks

    // --- Scoring ---
    int m_score{ 0 };
    int m_level{ 1 };
    int m_linesCleared{ 0 };

    // --- High score / leaderboard ---
    std::vector<HighScore> m_leaderboard;   // loaded at startup, kept sorted
    int m_newEntryIndex{ -1 };              // index of the just-inserted entry (-1 = none)
    std::string m_nameBuffer;               // player's name being typed

    // --- 7 bag system ---
    std::mt19937 m_gen{ std::random_device{}() };
    std::array<TetrominoType, 7> m_bag;
    int m_bagIndex{ 7 }; // start at 7 so the first call triggers a refill

    // --- Timing ---
    float fallTimer{ 0.0f };
    float dropMultiplier{ 1.0f }; // 1.0 = normal, higher = faster

    // --- DAS (Delayed Auto Shift) for left/right ---
    float leftRepeatTimer{ 0.0f };
    float rightRepeatTimer{ 0.0f };
    const float DAS_DELAY{ 0.20f };  // Initial delay before auto-repeat starts (200ms)
    const float DAS_REPEAT{ 0.05f }; // Repeat rate once holding (20 moves per second)

    // --- Lock Delay + Flash ---
    float lockDelayTimer{ 0.0f };
    float lockFlashTimer{ 0.0f };
    const float LOCK_DELAY{ 0.35f }; // 300 ms freeze after landing
    const float LOCK_FLASH_DURATION{ 0.2f }; // quick flash during lock delay
    bool isLocking{ false };
    bool isLockFlashing{ false };
    std::vector<glm::ivec2> justLockedCells; // for lock flash

    // --- Line Clear animation ---
    std::vector<int> clearingRows; // rows being cleared
    float clearAnimationTimer{ 0.0f };
    const float CLEAR_ANIMATION_DURATION{ 0.35f }; // 350 ms flash + shrink
    bool isClearing{ false };

    // --- Input de-bouncing ---
    bool lastUpPressed{ false };
    bool lastLeftPressed{ false };
    bool lastRightPressed{ false };
    bool lastHoldPressed{ false };
    bool lastEnterPressed{ false };     // confirm name / interact with pause menu
    bool lastBackspacePressed{ false }; // delete last character in name entry
    bool lastEscapePressed{ false };    // pause from playing
    bool lastQuitPressed{ false };

    // --- Helper Functions ---
    bool canMove(const glm::ivec2& newPosition, int newRotation) const;
    void lockCurrentPiece();
    void holdCurrentPiece();
    void checkForLineClears();
    void spawnNewPiece();
    void refillBag();
    void updateGhostPiece();
    float getLevelFallInterval() const;

    void resetBoard();      // clears board and resets scoring for a new game
    void triggerGameOver(); // decides whether to go to NameEntry or just reset
    void confirmName();     // called when player presses Enter in NameEntry

    // Phase-specific input handlers (called from handleInput)
    void handlePlayingInput(GLFWwindow* window, float deltaTime);
    void handleNameEntryInput(GLFWwindow* window);
    void handlePausedInput(GLFWwindow* window);
};

#endif // GAMESTATE_H_INCLUDED
