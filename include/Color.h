#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include <glm/glm.hpp>

enum class Color : uint8_t
{
    black,
    red,
    orange,
    yellow,
    green,
    blue,
    purple,
    cyan,
    white,
};

// Helper function for sending RGB colors to shader
// inline because it's in a header
inline glm::vec3 getColorRGB(Color c)
{
    switch (c)
    {
    case Color::black:  return {0.0f, 0.0f, 0.0f};
    case Color::red:    return {1.0f, 0.0f, 0.0f};
    case Color::orange: return {1.0f, 0.5f, 0.0f};
    case Color::yellow: return {1.0f, 1.0f, 0.0f};
    case Color::green:  return {0.0f, 1.0f, 0.0f};
    case Color::blue:   return {0.0f, 0.0f, 1.0f};
    case Color::purple: return {0.5f, 0.0f, 1.0f};
    case Color::cyan:   return {0.0f, 1.0f, 1.0f};
    case Color::white:  return {1.0f, 1.0f, 1.0f};
    default:            return {1.0f, 1.0f, 1.0f}; // white
    }
}

#endif // COLOR_H_INCLUDED
