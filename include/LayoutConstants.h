#ifndef LAYOUTCONSTANTS_H_INCLUDED
#define LAYOUTCONSTANTS_H_INCLUDED

namespace config
{
    // Board dimensions
    inline constexpr int BOARD_WIDTH    { 10 };
    inline constexpr int BOARD_HEIGHT   { 20 };
    inline constexpr float HUD_HEIGHT   { 3.0f };
    inline constexpr float TOTAL_HEIGHT { HUD_HEIGHT + static_cast<float>(BOARD_HEIGHT) };
    inline constexpr float TARGET_ASPECT{ static_cast<float>(BOARD_WIDTH) / TOTAL_HEIGHT };

    // HUD element center X positions (0=left edge, 1=right edge of total game area)
    inline constexpr float HUD_HOLD_CENTER_X  { 1.0f / 8.0f };
    inline constexpr float HUD_SCORE_CENTER_X { 3.0f / 8.0f };
    inline constexpr float HUD_LEVEL_CENTER_X { 5.0f / 8.0f };
    inline constexpr float HUD_NEXT_CENTER_X  { 7.0f / 8.0f };

    // Gameplay flags
    inline constexpr bool showGhost{ true };
}

#endif // LAYOUTCONSTANTS_H_INCLUDED
