#ifndef TETROMINO_H_INCLUDED
#define TETROMINO_H_INCLUDED

#include <array>
#include <glm/glm.hpp>
#include "Color.h"

enum class TetrominoType : uint8_t
{
    I, O, T, S, Z, J, L
};

struct Tetromino
{
    TetrominoType type;
    int rotation{ 0 };              // 0-3
    glm::ivec2 position{ 5, 0 };    // grid coords, starts at center top
    Color color;
};

// Returns 4 relative positions (x,y) for the given type and rotation
std::array<glm::ivec2, 4> getTetrominoBlocks(TetrominoType type, int rotation);

Color getTetrominoColor(TetrominoType type);

#endif // TETROMINO_H_INCLUDED
