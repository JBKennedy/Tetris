#ifndef RENDERFRAME_H_INCLUDED
#define RENDERFRAME_H_INCLUDED

#include "HighScore.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

// -----------------------------------------------------------------------
// GamePhase - drives which overlay (if any) the Renderer draws on top
// of the always-visible board.
// -----------------------------------------------------------------------
enum class GamePhase
{
    Playing,    // No overlay - normal gameplay
    Paused,     // Overlay: pause / options menu (future use)
    NameEntry,  // Overlay: player types their name after a qualifying score
    Options,    // Overlay: user-configurable settings
};

namespace render_data
{
    enum class TextureID
    {
        Detail,
        Ghost
    };

    struct CellRenderData
    {
        glm::ivec2 gridPos;
        glm::vec3  tint;
        float      scale;
        TextureID  texture;
    };

    // Data the Renderer needs for in-game UI elements
    struct UIRenderData
    {
        int score;
        int level;
        bool hasHold;
        std::vector<CellRenderData> holdPieceCells;
        std::vector<CellRenderData> nextPieceCells;
    };

    // Data the Renderer needs to draw any overlay.
    // Only the fields relevant to the current phase need to be populated.
    struct OverlayRenderData
    {
        // --- Paused (always populated when phase != Playing) ---
        std::vector<HighScore> leaderboard; // Sorted highest-first, shown on pause menu
        int         newEntryIndex{ -1 };    // Index of just-added entry, or -1
        bool        gameHasStarted{ false };// false = show "Play", true = show "Resume"

        // --- NameEntry ---
        std::string nameBuffer;             // Characters typed so far
        int         finalScore{ 0 };
        int         finalLevel{ 1 };
    };

    struct RenderFrame
    {
        std::vector<CellRenderData> cells;
        UIRenderData        ui;
        GamePhase           phase{ GamePhase::Playing };
        OverlayRenderData   overlay;
    };

} // namespace render_data

#endif // RENDERFRAME_H_INCLUDED
