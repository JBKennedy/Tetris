#include "Renderer.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <cstdio>   // for std::snprintf()
#include "LayoutConstants.h"

// future proofing for posability of handling multiple windows
static std::unordered_map<GLFWwindow*, Renderer*> s_instances;

Renderer::Renderer(GLFWwindow* window)
    : m_window{ window }
    , m_quadVAO{ 0 }
    , m_totalScaleVec{ glm::vec3(1.0f) }
    , m_boardModelMatrix{ glm::mat4(1.0f) }
    , m_shader{ "shaders/shader.vert.glsl", "shaders/shader.frag.glsl" }
    , m_texCache{}
    , m_gridOverlay{ nullptr }
    , m_detailTex{ nullptr }
    , m_ghostTex{ nullptr }
    , m_asciiTex{ nullptr }
    , m_solidTex{ nullptr }
{
    // set callback function for window resizing
    s_instances[m_window] = this;
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    // create quad
    m_quadVAO = createQuadVAO();

    // load textures
    m_gridOverlay = &m_texCache.get("textures/grid_overlay.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, false);
    m_detailTex   = &m_texCache.get("textures/detailTex.png",
                                GL_REPEAT, GL_REPEAT,
                                GL_LINEAR, GL_LINEAR, true);
    m_ghostTex    = &m_texCache.get("textures/ghostTex.png",
                                GL_REPEAT, GL_REPEAT,
                                GL_LINEAR, GL_LINEAR, true);
    m_digitsTex   = &m_texCache.get("textures/digits.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, true);
    m_labelsTex   = &m_texCache.get("textures/labels.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, true);
    m_uiBoxTex    = &m_texCache.get("textures/ui_box.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, false);
    m_asciiTex    = &m_texCache.get("textures/ascii_sprites.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, true);
    m_solidTex    = &m_texCache.get("textures/solid_white.png",
                                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                GL_LINEAR, GL_LINEAR, false);
    m_woodTex     = &m_texCache.get("textures/wood_pine.png",
                                GL_REPEAT, GL_REPEAT,
                                GL_LINEAR, GL_LINEAR, true);

    // initial board matrix
    updateBoardModelMatrix();
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_quadVAO);
    s_instances.erase(m_window);
}

unsigned int Renderer::createQuadVAO()
{
    constexpr float vertices[]
    {
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f
    };
    constexpr unsigned int indices[]{ 0, 1, 2, 2, 3, 0 };

    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO); // initialize ID for VAO
    glGenBuffers(1, &VBO);      // ----------------- VBO
    glGenBuffers(1, &EBO);      // ----------------- EBO

    glBindVertexArray(VAO);     // must be bound first

    // Vertex Buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Element Buffer (indices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return VAO;
}

void Renderer::draw(const render_data::RenderFrame& frame, float deltaTime)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // teal
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader.use();
    m_shader.setFloat("opacity", 1.0f); // default fully opaque — overlays override per-draw

    // Draw wood background (tiled - board is 2:1 aspect, wood tile is 1:1)
    m_woodTex->bind(GL_TEXTURE0);
    m_shader.setMat4("transform", m_boardModelMatrix);
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setBool("useAlpha", false);
    m_shader.setBool("useSpriteSheet", true);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f, 2.0f));  // tile vertically to fill board
    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Draw grid overlay
    m_gridOverlay->bind(GL_TEXTURE0);
    m_shader.setMat4("transform", m_boardModelMatrix);
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setBool("useAlpha", true);
    m_shader.setBool("useSpriteSheet", false);               // sample RGBA directly - ugly hack, should improve shader instead
    m_shader.setFloat("opacity", 1.0f);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));    // sample full texture
    m_shader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));     //
    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Draw all cells from the render frame
    const float cellWidth{ 1.0f / static_cast<float>(config::BOARD_WIDTH) };
    const float cellHeight{ 1.0f / static_cast<float>(config::BOARD_HEIGHT) };

    for (const auto& cell : frame.cells)
    {
        glm::mat4 model{ m_boardModelMatrix };

        float cellX{ (cell.gridPos.x * cellWidth) - 0.5f + (cellWidth * 0.5f) };
        float cellY{ 0.5f - (cell.gridPos.y * cellHeight) - (cellHeight * 0.5f) };
        model = glm::translate(model, glm::vec3(cellX, cellY, 0.0f));
        model = glm::scale(model, glm::vec3(cellWidth * 0.95f * cell.scale,
                                            cellHeight * 0.95f * cell.scale, 1.0f));

        m_shader.setMat4("transform", model);
        m_shader.setVec3("tintColor", cell.tint);
        m_shader.setBool("useAlpha", true);
        m_shader.setBool("useSpriteSheet", false);
        m_shader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));    // sample full texture
        m_shader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));     //

        if (cell.texture == render_data::TextureID::Detail)
            m_detailTex->bind(GL_TEXTURE0);
        else
            m_ghostTex->bind(GL_TEXTURE0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    drawHUD(frame.ui);

    // Draw overlay on top of board if not Playing
    if (frame.phase == GamePhase::Paused)
        drawPauseOverlay(frame.overlay);
    else if (frame.phase == GamePhase::NameEntry)
        drawNameEntryOverlay(frame.overlay, deltaTime);
}
// needs to be called once after window creation and also every time  the window resizes
// -------------------------------------------------------------------------------------
void Renderer::updateBoardModelMatrix()
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    float windowAspect{ static_cast<float>(width) / height };

    // Compute m_totalScaleVec so the total 23-unit game area
    // fits the window while preserving aspect ratio
    if (windowAspect > config::TARGET_ASPECT)
    {   // Window wider → scale to match height, add black bars on sides
        float fitHeight{ static_cast<float>(height) };
        float fitWidth{ fitHeight * config::TARGET_ASPECT };
        m_totalScaleVec.x = fitWidth / width;  // normalized to [-1,1]
        m_totalScaleVec.y = fitHeight / height;
    }
    else
    {   // Window taller → scale to match width, black bars top/bottom
        float fitWidth{ static_cast<float>(width) };
        float fitHeight{ fitWidth / config::TARGET_ASPECT };
        m_totalScaleVec.x = fitWidth / width;
        m_totalScaleVec.y = fitHeight / height;
    }

    // ======================= Position Board =======================
    // boardTop = clip-space Y where the HUD ends and the board begins
    float boardTop{ m_totalScaleVec.y - (config::HUD_HEIGHT / config::TOTAL_HEIGHT) * 2.0f * m_totalScaleVec.y };
    // boardBottom = bottom of the total game area in clip space
    float boardBottom{ -m_totalScaleVec.y };
    // boardOffsetY = midpoint between boardTop and the bottom of the game area
    float boardOffsetY{ (boardTop + boardBottom) / 2.0f };
    // Apply offset to shift the game board down
    m_boardModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, boardOffsetY, 0.0f));
    // ======================== Scale Board =========================
    // Start from m_totalScaleVec — will be adjusted to board's proportion below
    glm::vec3 boardScaleVec{ m_totalScaleVec };
    // Shrink boardScaleVec.y to just the board's proportion of the total area
    boardScaleVec.y *= static_cast<float>(config::BOARD_HEIGHT) / config::TOTAL_HEIGHT;
    // Apply scale (multiply by 2 because quad is -0.5 to 0.5 → full screen = 2.0 range)
    m_boardModelMatrix = glm::scale(m_boardModelMatrix, boardScaleVec * 2.0f);

    // =============== Position & Scale HUD elements ===============
    // Same for all HUD elements
    float hudCenterY{ (m_totalScaleVec.y + boardTop) / 2.0f };
    glm::vec3 elementScale{
        m_totalScaleVec.x * 2.0f * (1.0f / 4.0f), // quarter of total width
        m_totalScaleVec.y * 2.0f * (config::HUD_HEIGHT / config::TOTAL_HEIGHT), // HUD height fraction
        1.0f };
    // HOLD
    float holdCenterX{ (config::HUD_HOLD_CENTER_X - 0.5f) * m_totalScaleVec.x * 2.0f };
    m_holdMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(holdCenterX, hudCenterY, 0.0f));
    m_holdMatrix = glm::scale(m_holdMatrix, elementScale);
    // SCORE
    float scoreCenterX{ (config::HUD_SCORE_CENTER_X - 0.5f) * m_totalScaleVec.x * 2.0f };
    m_scoreMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(scoreCenterX, hudCenterY, 0.0f));
    m_scoreMatrix = glm::scale(m_scoreMatrix, elementScale);
    // LEVEL
    float levelCenterX{ (config::HUD_LEVEL_CENTER_X - 0.5f) * m_totalScaleVec.x * 2.0f };
    m_levelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(levelCenterX, hudCenterY, 0.0f));
    m_levelMatrix = glm::scale(m_levelMatrix, elementScale);
    // NEXT
    float nextCenterX{ (config::HUD_NEXT_CENTER_X - 0.5f) * m_totalScaleVec.x * 2.0f };
    m_nextMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(nextCenterX, hudCenterY, 0.0f));
    m_nextMatrix = glm::scale(m_nextMatrix, elementScale);
}
// whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays
    glViewport(0, 0, width, height);

    auto it{ s_instances.find(window) };
    if (it != s_instances.end())
        it->second->updateBoardModelMatrix();
}

glm::mat4 Renderer::elementRelativeTransform(const glm::mat4& elementMatrix,
                                             float xFraction, float yFraction,
                                             float widthFraction, float heightFraction)
{
    // Extract clip-space center and half-size from element matrix
    glm::vec3 center{ elementMatrix[3] };
    glm::vec3 elementSize{ glm::length(elementMatrix[0]),
                           glm::length(elementMatrix[1]),
                           1.0f };

    // Convert fractions to clip-space positions and scale
    float x{ center.x + (xFraction - 0.5f) * elementSize.x };
    float y{ center.y + (yFraction - 0.5f) * elementSize.y };

    glm::mat4 result{ glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) };
    result = glm::scale(result, glm::vec3(elementSize.x * widthFraction,
                                          elementSize.y * heightFraction,
                                          1.0f));
    return result;
}

void Renderer::drawHUD( const render_data::UIRenderData& ui )
{
    glBindVertexArray(m_quadVAO);
    m_shader.setBool("useAlpha", true);                  // UI textures have alpha
    m_shader.setBool("useSpriteSheet", true);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f)); // sample full texture
    m_shader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));  //

    // ================== Draw Background Boxes ==================
    m_uiBoxTex->bind(GL_TEXTURE0);
    m_shader.setVec3("tintColor", glm::vec3(1.0f)); // no tint

    m_shader.setMat4("transform", m_holdMatrix);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    m_shader.setMat4("transform", m_scoreMatrix);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    m_shader.setMat4("transform", m_levelMatrix);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    m_shader.setMat4("transform", m_nextMatrix);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    // ========== Draw Labels (SCORE, LEVEL, NEXT, HOLD) ==========
    m_labelsTex->bind(GL_TEXTURE0);
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setBool("useSpriteSheet", true);
    // Inset 1/2 a pixel when sampling spriteSheet to avoid sample bleeding
    const float labelSheetWidth{ 512.0f };
    const float labelUVInset{ 0.5f / labelSheetWidth };

    // HOLD label — sprite sheet index 3
    m_shader.setVec2("uvOffset", glm::vec2(3.0f / 4.0f + labelUVInset, 0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f / 4.0f - (2.0f * labelUVInset), 1.0f));
    // centered horizontally (0.5), upper quarter vertically (0.75), 80% wide, 40% tall
    m_shader.setMat4("transform", elementRelativeTransform(m_holdMatrix, 0.5f, 0.75f, 0.8f, 0.4f));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // SCORE label — sprite sheet index 0
    m_shader.setVec2("uvOffset", glm::vec2(0.0f / 4.0f + labelUVInset, 0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f / 4.0f - (2.0f * labelUVInset), 1.0f));
    // centered horizontally (0.5), upper quarter vertically (0.75), 80% wide, 40% tall
    m_shader.setMat4("transform", elementRelativeTransform(m_scoreMatrix, 0.5f, 0.75f, 0.8f, 0.4f));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // LEVEL label — sprite sheet index 1
    m_shader.setVec2("uvOffset", glm::vec2(1.0f / 4.0f + labelUVInset, 0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f / 4.0f - (2.0f * labelUVInset), 1.0f));
    // centered horizontally (0.5), upper quarter vertically (0.75), 80% wide, 40% tall
    m_shader.setMat4("transform", elementRelativeTransform(m_levelMatrix, 0.5f, 0.75f, 0.8f, 0.4f));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // NEXT label — sprite sheet index 2
    m_shader.setVec2("uvOffset", glm::vec2(2.0f / 4.0f + labelUVInset, 0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f / 4.0f - (2.0f * labelUVInset), 1.0f));
    // centered horizontally (0.5), upper quarter vertically (0.75), 80% wide, 40% tall
    m_shader.setMat4("transform", elementRelativeTransform(m_nextMatrix, 0.5f, 0.75f, 0.8f, 0.4f));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // ========== Draw Digits ==========
    // Score
    glm::vec3 scoreCenter{ m_scoreMatrix[3] };
    drawDigits(ui.score, 7, scoreCenter.x, scoreCenter.y);

    // Level
    glm::vec3 levelCenter{ m_levelMatrix[3] };
    drawDigits(ui.level, 3, levelCenter.x, levelCenter.y);

    // ========== Draw NEXT & HOLD piece previews ==========
    m_detailTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha", false); // reset to opaque
    m_shader.setBool("useSpriteSheet", false);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f)); // normalize
    m_shader.setVec2("uvScale",  glm::vec2(1.0f, 1.0f)); // normalize

    constexpr float cellSize{ 0.18f };       // fraction of box width per cell
    constexpr float previewYCenter{ 0.35f }; // 0=bottom of box, 1=top

    // Get actual framebuffer size to convert clip space → pixels
    int fbW, fbH;
    glfwGetFramebufferSize(m_window, &fbW, &fbH);

    // Box dimensions in clip space
    float boxClipW{ glm::length(glm::vec3(m_nextMatrix[0])) };
    float boxClipH{ glm::length(glm::vec3(m_nextMatrix[1])) };

    // Convert box to pixels, compute a square cell size in pixels, convert back to clip space
    float boxPixW{ boxClipW * fbW * 0.5f };
    float boxPixH{ boxClipH * fbH * 0.5f };
    float cellPixSize{ boxPixW * cellSize };            // square in pixels
    float cellClipX{ cellPixSize / (fbW * 0.5f) };      // back to clip space X
    float cellClipY{ cellPixSize / (fbH * 0.5f) };      // back to clip space Y (differs from X)

    // Fractions of the box these clip-space sizes represent
    float cellFracX{ cellClipX / boxClipW };
    float cellFracY{ cellClipY / boxClipH };

    auto drawPreview = [&](const glm::mat4& boxMatrix,
                           const std::vector<render_data::CellRenderData>& cells)
    {
        glm::vec3 boxCenter{ boxMatrix[3] };
        for (const auto& cell : cells)
        {
            m_shader.setVec3("tintColor", cell.tint);

            // Position using clip-space step sizes that are equal in pixels
            float x{ boxCenter.x + (cell.gridPos.x + 0.5f) * cellClipX };
            float y{ boxCenter.y + (previewYCenter - 0.5f) * boxClipH
                                 + (-cell.gridPos.y + 0.5f) * cellClipY };

            glm::mat4 model{ glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) };
            model = glm::scale(model, glm::vec3(cellClipX * 0.95f, cellClipY * 0.95f, 1.0f));
            m_shader.setMat4("transform", model);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    };

    drawPreview(m_nextMatrix, ui.nextPieceCells);
    if (ui.hasHold)
        drawPreview(m_holdMatrix, ui.holdPieceCells);
}

// =========================================================================
// checkClick() — converts a GLFW pixel coordinate to clip space and tests
// against stored button rects. Call this from the mouse button callback.
MenuButton Renderer::checkClick(double mouseX, double mouseY) const
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);

    // GLFW pixel origin is top-left; clip space origin is centre with Y up
    float clipX{  (static_cast<float>(mouseX) / w) * 2.0f - 1.0f };
    float clipY{ -(static_cast<float>(mouseY) / h) * 2.0f + 1.0f };
    glm::vec2 p{ clipX, clipY };

    if (m_playButtonRect.contains(p)) return MenuButton::Play;
    if (m_quitButtonRect.contains(p)) return MenuButton::Quit;
    return MenuButton::None;
}

// =========================================================================
// checkHover() — converts the current cursor position to clip space and
// updates m_hoveredButton. Call from the GLFW cursor-position callback.
void Renderer::checkHover(double mouseX, double mouseY)
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);

    float clipX{  (static_cast<float>(mouseX) / w) * 2.0f - 1.0f };
    float clipY{ -(static_cast<float>(mouseY) / h) * 2.0f + 1.0f };
    glm::vec2 p{ clipX, clipY };

    if      (m_playButtonRect.contains(p)) m_hoveredButton = MenuButton::Play;
    else if (m_quitButtonRect.contains(p)) m_hoveredButton = MenuButton::Quit;
    else                                   m_hoveredButton = MenuButton::None;
}

// =========================================================================
// drawText() — renders a string using the ascii sprite sheet.
// centerX/centerY are clip-space coordinates for the string's center.
// charWidth/charHeight are clip-space sizes per character quad.
void Renderer::drawText(const std::string& text, float centerX, float centerY,
                        float charWidth, float charHeight, glm::vec3 tint)
{
    if (text.empty()) return;

    // ascii_sprites layout: 16 cols x 6 rows, codepoint 32 ('space') = index 0
    constexpr float COLS{ 16.0f };
    constexpr float ROWS{ 6.0f };
    constexpr float uvW { 1.0f / COLS };
    constexpr float uvH { 1.0f / ROWS };

    m_asciiTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha", true);
    m_shader.setBool("useSpriteSheet", true);
    m_shader.setVec3("tintColor", tint);

    float totalWidth{ static_cast<float>(text.size()) * charWidth };
    float startX    { centerX - totalWidth * 0.5f + charWidth * 0.5f };

    for (int i{ 0 }; i < static_cast<int>(text.size()); ++i)
    {
        unsigned char cp{ static_cast<unsigned char>(text[i]) };
        // clamp to printable ASCII range
        if (cp < 32 || cp > 126) cp = '?';
        int index{ cp - 32 };

        float uOff{ (index % 16) * uvW };
        float vOff{ (index / 16) * uvH };
        m_shader.setVec2("uvOffset", glm::vec2(uOff, vOff));
        m_shader.setVec2("uvScale", glm::vec2(uvW, uvH));

        float x{ startX + i * charWidth };
        glm::mat4 model{ glm::translate(glm::mat4(1.0f), glm::vec3(x, centerY, 0.0f)) };
        model = glm::scale(model, glm::vec3(charWidth, charHeight, 1.0f));
        m_shader.setMat4("transform", model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Reset sprite sheet state so callers don't have to
    m_shader.setBool("useSpriteSheet", false);
    m_shader.setBool("useAlpha", false);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f));
}

// =========================================================================
// drawPauseOverlay() — semi-transparent panel with title, buttons, leaderboard
void Renderer::drawPauseOverlay(const render_data::OverlayRenderData& overlay)
{
    glBindVertexArray(m_quadVAO);

    // ---- Dim the board - black quad at 60% opacity over the full game area ----
    m_solidTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha",       true);
    m_shader.setBool("useSpriteSheet", true);
    m_shader.setVec2("uvOffset",       glm::vec2(0.0f));
    m_shader.setVec2("uvScale",        glm::vec2(1.0f));
    m_shader.setVec3("tintColor",      glm::vec3(0.0f)); // black
    m_shader.setFloat("opacity",       0.6f);

    glm::mat4 dimModel{ glm::scale(glm::mat4(1.0f),
                                   glm::vec3(m_totalScaleVec.x * 2.0f,
                                             m_totalScaleVec.y * 2.0f, 1.0f)) };
    m_shader.setMat4("transform", dimModel);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Reset opacity for everything that follows
    m_shader.setFloat("opacity", 1.0f);

    // ---- Panel box ----
    float panelW{ m_totalScaleVec.x * 2.0f * 0.75f };    // original 0.6f
    float panelH{ m_totalScaleVec.y * 2.0f * 0.5f };   // original 0.85f
    glm::mat4 panelModel{ glm::scale(glm::mat4(1.0f),
                                     glm::vec3(panelW, panelH, 1.0f)) };

    m_uiBoxTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha", true);
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setMat4("transform", panelModel);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Character sizes scaled relative to panel width
    // Sprite sheet cell aspect ratio: 24w x 40h
    const float charW{ panelW * 0.045f };   // original 0.045f
    const float charH{ charW * (40.0f / 24.0f) * 1.8f };
    const float bigCharW{ charW * 1.6f };
    const float bigCharH{ bigCharW * (40.0f / 24.0f) * 1.8f };

    // ---- Title ----
    const std::string title{ overlay.gameHasStarted ? "PAUSED" : "TETRIS" };
    float titleY{ panelH * 0.5f - bigCharH };
    drawText(title, 0.0f, titleY, bigCharW, bigCharH, glm::vec3(1.0f, 0.8f, 0.0f));

    // ---- Play / Resume button ----
    const std::string playLabel{ overlay.gameHasStarted ? "RESUME" : "PLAY" };
    float playY{ titleY - bigCharH * 1.8f };

    bool playHovered{ m_hoveredButton == MenuButton::Play };
    // Hovered: full bright green + slightly larger; idle: dimmer green
    glm::vec3 playTint{ playHovered ? glm::vec3(0.6f, 1.0f, 0.6f) : glm::vec3(0.4f, 1.0f, 0.4f) };
    float     playScale{ playHovered ? 1.15f : 1.0f };
    drawText(playLabel, 0.0f, playY,
             charW * playScale, charH * playScale, playTint);

    m_playButtonRect.center   = { 0.0f, playY }; // PLAY button rect for hit testing
    m_playButtonRect.halfSize = { panelW * 0.4f, charH * 1.1f }; // slightly generous click target

    // ---- Quit button ----
    float quitY{ playY - charH * 1.8f };

    bool quitHovered{ m_hoveredButton == MenuButton::Quit };
    glm::vec3 quitTint{ quitHovered ? glm::vec3(1.0f, 0.6f, 0.6f) : glm::vec3(1.0f, 0.4f, 0.4f) };
    float     quitScale{ quitHovered ? 1.15f : 1.0f };
    drawText("QUIT", 0.0f, quitY,
             charW * quitScale, charH * quitScale, quitTint);

    m_quitButtonRect.center   = { 0.0f, quitY }; // QUIT button rect for hit testing
    m_quitButtonRect.halfSize = { panelW * 0.4f, charH * 1.1f };

    // ---- Leaderboard heading ----
    float lbTopY{ quitY - charH * 2.5f };
    drawText("HIGH SCORES", 0.0f, lbTopY, charW, charH, glm::vec3(1.0f, 0.8f, 0.0f));

    // ---- Column header ----
    float headerY{ lbTopY - charH * 1.6f };
    drawText("#  NAME      SCORE  LVL", 0.0f, headerY,
             charW * 0.85f, charH * 0.85f, glm::vec3(0.7f));

    // Entries
    for (int i{ 0 }; i < static_cast<int>(overlay.leaderboard.size()); ++i)
    {
        const HighScore& hs{ overlay.leaderboard[i] };
        glm::vec3 entryTint{ i == overlay.newEntryIndex
                             ? glm::vec3(1.0f, 0.9f, 0.2f)
                             : glm::vec3(1.0f) };

        char row[48]{};
        std::snprintf(row, sizeof(row), "%-1d  %-8s  %5d  %3d",
                      i + 1, hs.name.c_str(), hs.score, hs.level);

        float entryY{ headerY - charH * (1.4f + i * 1.3f) };
        drawText(row, 0.0f, entryY, charW * 0.85f, charH * 0.85f, entryTint);
    }
}

// =========================================================================
// drawNameEntryOverlay() — prompts player to type their name
void Renderer::drawNameEntryOverlay(const render_data::OverlayRenderData& overlay, float deltaTime)
{
    m_cursorTimer += deltaTime;

    glBindVertexArray(m_quadVAO);

    // ---- Dim the board ----
    m_solidTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha",       true);
    m_shader.setBool("useSpriteSheet", false);
    m_shader.setVec2("uvOffset",       glm::vec2(0.0f));
    m_shader.setVec2("uvScale",        glm::vec2(1.0f));
    m_shader.setVec3("tintColor",      glm::vec3(0.0f));
    m_shader.setFloat("opacity",       0.6f);

    glm::mat4 dimModel{ glm::scale(glm::mat4(1.0f),
                                   glm::vec3(m_totalScaleVec.x * 2.0f,
                                             m_totalScaleVec.y * 2.0f, 1.0f)) };
    m_shader.setMat4("transform", dimModel);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    m_shader.setFloat("opacity", 1.0f);

    // ---- Panel box ----
    float panelW{ m_totalScaleVec.x * 2.0f * 0.95f };
    float panelH{ m_totalScaleVec.y * 2.0f * 0.35f };
    glm::mat4 panelModel{ glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) };
    panelModel = glm::scale(panelModel, glm::vec3(panelW, panelH, 1.0f));

    m_uiBoxTex->bind(GL_TEXTURE0);
    m_shader.setBool("useAlpha", true);
    m_shader.setBool("useSpriteSheet", true);
    m_shader.setVec2("uvOffset", glm::vec2(0.0f));
    m_shader.setVec2("uvScale",  glm::vec2(1.0f));
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setMat4("transform", panelModel);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    const float charW{ panelW * 0.045f };
    const float charH{ charW * (40.0f / 24.0f) * 1.8f };
    const float bigCharW{ charW * 1.3f };
    const float bigCharH{ bigCharW * (40.0f / 24.0f) * 1.8f };

    // ---- Heading ----
    float headingY{ panelH * 0.5f - bigCharH };
    drawText("NEW HIGH SCORE!", 0.0f, headingY, bigCharW, bigCharH,
             glm::vec3(1.0f, 0.8f, 0.0f));

    // ---- Final score and level ----
    char scoreStr[32]{};
    std::snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d  LEVEL: %d",
                  overlay.finalScore, overlay.finalLevel);
    float scoreY{ headingY - bigCharH * 1.6f };
    drawText(scoreStr, 0.0f, scoreY, charW * 0.9f, charH * 0.9f, glm::vec3(0.9f));

    // ---- Name input label ----
    float labelY{ scoreY - charH * 2.2f };
    drawText("ENTER YOUR NAME:", 0.0F, labelY, charW, charH, glm::vec3(0.8f));

    // ---- Name buffer + blinking cursor ----
    // Cursor blinks at ~1 Hz (0.5s on, 0.5s off)
    bool cursorVisible{ std::fmod(m_cursorTimer, 1.0f) < 0.5f };

    const float nameCharW{ charW * 1.2f };
    const float nameCharH{ charH * 1.2f };
    float nameY{ labelY - charH * 1.8f };

    // Draw the name centered half a character to the left,
    // leaving a reserved slot on the right for the cursor
    float nameCenterX{ -nameCharW * 0.5f };
    drawText(overlay.nameBuffer, nameCenterX, nameY, nameCharW, nameCharH, glm::vec3(1.0f));

    // Draw cursor at the fixed slot just right of the name's centered position

    if (cursorVisible)
    {
        float nameHalfWidth{ overlay.nameBuffer.size() * nameCharW * 0.5f };
        float cursorX{ nameCenterX + nameHalfWidth + nameCharW * 0.5f };
        drawText("|", cursorX, nameY, nameCharW, nameCharH, glm::vec3(1.0f));
    }

    // ---- Hint ----
    float hintY{ nameY - charH * 2.0f };
    drawText("[ENTER] CONFIRM  [BKSP] DELETE", 0.0f, hintY,
             charW * 0.55f, charH * 0.55f, glm::vec3(0.5f));
}

void Renderer::drawDigits(int value, int maxDigits, float centerX, float centerY)
{
    // Clamp value to maxDigits
    int maxValue{ static_cast<int>(std::pow(10, maxDigits)) - 1 };
    value = std::min(value, maxValue);

    // Calculate digit count
    int digitCount{ 1 };
    if(value > 0)
        digitCount = static_cast<int>(std::log10(value)) + 1;

    // Digit quad size in clip space
    float digitWidth{ m_totalScaleVec.x * 2.0f * (1.0f / 4.0f) * 0.1f };
    float digitHeight{ digitWidth * 10 * (m_totalScaleVec.x / m_totalScaleVec.y) };
    // (64.0f / 32.0f) became 10 why?
    // Starting X so digits are centered
    float totalWidth{ digitCount * digitWidth };
    float startX{ centerX - totalWidth / 2.0f + digitWidth / 2.0f };

    m_digitsTex->bind(GL_TEXTURE0);
    m_shader.setVec3("tintColor", glm::vec3(1.0f));
    m_shader.setBool("useSpriteSheet", true);

    // Extract digits and draw right to left
    for (int i{ digitCount - 1 }; i >= 0; --i)
    {
        int digit{ value % 10 };
        value /= 10;

        float uvLeft{ static_cast<float>(digit) / 10.0f };
        m_shader.setVec2("uvOffset", glm::vec2(uvLeft, 0.0f));
        m_shader.setVec2("uvScale",glm::vec2(1.0f / 10.0f, 1.0f));

        float x{ startX + i * digitWidth };
        glm::mat4 model{ glm::translate(glm::mat4(1.0f), glm::vec3(x, centerY, 0.0f)) };
        model = glm::scale(model, glm::vec3(digitWidth, digitHeight, 1.0f));
        m_shader.setMat4("transform", model);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    m_shader.setBool("useSpriteSheet", false);
}
