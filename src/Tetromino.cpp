#include "Tetromino.h"
#include <array>
#include <glm/glm.hpp>

// Returns 4 relative positions (x,y) for the given type and rotation
// ------------------------------------------------------------------
std::array<glm::ivec2, 4> getTetrominoBlocks(TetrominoType type, int rotation)
{
    rotation = rotation % 4;    // normalize to 0-3

    switch (type)
    {
    case TetrominoType::I:  // long piece
        if (rotation == 0 || rotation == 2)         // horizontal
            return {{{-1,0}, {0,0},{1,0},{2,0}}};
        else                                        // vertical
            return {{{0,-1}, {0,0}, {0,1}, {0,2}}};
        break;

    case TetrominoType::O:  // square - no rotation
        return {{{0,0}, {1,0}, {0,1}, {1,1}}};
        break;

    case TetrominoType::T:
        if (rotation == 0) return {{{-1,0}, {0,0}, {1,0}, {0,1}}};  // point up
        if (rotation == 1) return {{{0,-1}, {0,0}, {0,1}, {1,0}}};  // point right
        if (rotation == 2) return {{{-1,0}, {0,0}, {1,0}, {0,-1}}}; // point down
        if (rotation == 3) return {{{0,-1}, {0,0}, {0,1}, {-1,0}}}; // point left
        break;

    case TetrominoType::S:
        if (rotation == 0 || rotation == 2) return {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}};
        else                                return {{{0, -1}, {0, 0}, {1, 0}, {1, 1}}};
        break;

    case TetrominoType::Z:
        if (rotation == 0 || rotation == 2) return {{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}};
        else                                return {{{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}};
        break;

    case TetrominoType::J:
        if (rotation == 0) return {{{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}};
        if (rotation == 1) return {{{0, -1}, {0, 0}, {0, 1}, {-1, 1}}};
        if (rotation == 2) return {{{-1, 0}, {0, 0}, {1, 0}, {1, -1}}};
        if (rotation == 3) return {{{1, -1}, {0, -1}, {0, 0}, {0, 1}}};
        break;

    case TetrominoType::L:
        if (rotation == 0) return {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}};
        if (rotation == 1) return {{{0, -1}, {0, 0}, {0, 1}, {1, -1}}};
        if (rotation == 2) return {{{-1, -1}, {-1, 0}, {0, 0}, {1, 0}}};
        if (rotation == 3) return {{{-1, 1}, {0, -1}, {0, 0}, {0, 1}}};
        break;

    default:
        return {{{0,0},{0,0},{0,0},{0,0}}};
    }
}

Color getTetrominoColor(TetrominoType type)
{
    switch (type)
    {
    case TetrominoType::I: return Color::cyan;
    case TetrominoType::O: return Color::yellow;
    case TetrominoType::T: return Color::purple;
    case TetrominoType::S: return Color::green;
    case TetrominoType::Z: return Color::red;
    case TetrominoType::J: return Color::blue;
    case TetrominoType::L: return Color::orange;
    default:               return Color::white;
    }
}
