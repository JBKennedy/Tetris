#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <string>

#include "shader_class/shader_class.h"
#include "texture_class/texture_class.h"
#include "textureCache_class/textureCache_class.h"
#include "RenderFrame.h"

// Identifies which pause menu button was clicked, or None if no hit
enum class MenuButton
{
    None,
    Play,   // "Play" or "Resume"
    Quit,
};

class Renderer
{
public:
    Renderer(GLFWwindow* window);
    ~Renderer();

    Renderer(const Renderer&) = delete;             // copy
    Renderer& operator=(const Renderer&) = delete;  // copy assignment
    Renderer(Renderer&&) = delete;                  // move
    Renderer& operator=(Renderer&&) = delete;       // move assignment

    void draw(const render_data::RenderFrame& frame, float deltaTime);

    // Convert a GLFW pixel click to clip space and test against menu buttons.
    // Only meaningful when phase == Paused; returns None otherwise.
    MenuButton checkClick(double mouseX, double mouseY) const;

    // Update the hovered button from the current cursor position.
    // Call from the GLFW cursor-position callback every frame.
    void checkHover(double mouseX, double mouseY);

private:
    GLFWwindow*     m_window;
    unsigned int    m_quadVAO;
    glm::vec3       m_totalScaleVec;
    glm::mat4       m_boardModelMatrix;
    glm::mat4       m_holdMatrix;
    glm::mat4       m_scoreMatrix;
    glm::mat4       m_levelMatrix;
    glm::mat4       m_nextMatrix;

    Shader          m_shader;
    TextureCache    m_texCache;
    const Texture*  m_detailTex;
    const Texture*  m_gridOverlay;
    const Texture*  m_ghostTex;
    const Texture*  m_digitsTex;
    const Texture*  m_labelsTex;
    const Texture*  m_uiBoxTex;
    const Texture*  m_asciiTex;
    const Texture*  m_solidTex;     // 1x1 white RGBA — used for tinted/dimmed quads
    const Texture*  m_woodTex;      // game board background

    float           m_cursorTimer{ 0.0f }; // drives name-entry cursor blink

    // Axis-aligned button rect in clip space - set during drawPauseOverlay,
    // read during checkClick. Bother are in the same coordinate system so no
    // conversion is needed between drawing and hit testing.
    struct ButtonRect
    {
        glm::vec2 center{ 0.0f };
        glm::vec2 halfSize{ 0.0f };

        bool contains(glm::vec2 p) const
        {
            return std::abs(p.x - center.x) <= halfSize.x
                && std::abs(p.y - center.y) <= halfSize.y;
        }
    };

    ButtonRect m_playButtonRect;
    ButtonRect m_quitButtonRect;
    MenuButton m_hoveredButton{ MenuButton::None }; // updated by checkHover() each frame

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void updateBoardModelMatrix();
    unsigned int createQuadVAO();
    glm::mat4 elementRelativeTransform(const glm::mat4& elementMatrix,
                                       float xFraction, float yFraction,
                                       float widthFraction, float heightFraction);
    void drawHUD(const render_data::UIRenderData& ui);
    void drawDigits(int value, int maxDigits, float centerX, float centerY);
    void drawText(const std::string& text, float centerX, float centerY,
                  float charWidth, float charHeight,
                  glm::vec3 tint = glm::vec3(1.0f));
    void drawPauseOverlay(const render_data::OverlayRenderData& overlay);
    void drawNameEntryOverlay(const render_data::OverlayRenderData& overlay, float deltaTime);
};

#endif // RENDERER_H_INCLUDED
