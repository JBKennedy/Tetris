#include "GameState.h"
#include "HighScore.h"
#include "LayoutConstants.h"
#include <iostream>
#include <random>

// === Constructor ===
GameState::GameState()
{
    m_leaderboard = loadHighScores();
    spawnNewPiece();
}

// only advances gameplay when phase == Playing
void GameState::update(GLFWwindow* window, float deltaTime)
{
    if (m_phase != GamePhase::Playing)
        return; // overlays are purely input-driven, nothing to tick

    // Lock delay / flash
    if (isLocking)
    {
        lockDelayTimer += deltaTime;
        lockFlashTimer += deltaTime;

        if (lockDelayTimer >= LOCK_DELAY)
        {
            lockDelayTimer = 0.0f;
            lockFlashTimer = 0.0f;
            isLocking = false;
            isLockFlashing = false;
            justLockedCells.clear();
            if (!isClearing)
                spawnNewPiece(); // only spawn here if no line clear is pending
        }
        return; // skip normal falling during lock delay
    }

    // --- Line clear animation (flash + shrink) ---
    if (isClearing)
    {
        clearAnimationTimer += deltaTime;

        if (clearAnimationTimer >= CLEAR_ANIMATION_DURATION)
        {
            // Animation finished → now shift the board down
            for (int i{ 0 }; i < static_cast<int>(clearingRows.size()); ++i)
            {
                // Shift index of row to be cleared to compensate for previous shifts
                int clearedY{ clearingRows[i] + i }; // Fixes a row clearing bug
                for (int y{ clearedY }; y > 0; --y)
                {
                    for (int x{ 0 }; x < config::BOARD_WIDTH; ++x)
                    {
                        board[y][x] = board[y - 1][x];
                    }
                }
                // Clear the top row
                for ( int x{ 0 }; x < config::BOARD_WIDTH; ++x)
                {
                    board[0][x] = Color::black;
                }
            }

            clearingRows.clear();
            isClearing = false;
            spawnNewPiece(); // always the single true spawn after a line clear
        }
        return; // Skip normal falling during line clear animation
    }

    // --- Normal falling ---
    fallTimer += deltaTime * dropMultiplier;// multiplier for stable fast drop

    if (fallTimer >= getLevelFallInterval())
    {
        glm::ivec2 newPos{ currentPiece.position };
        newPos.y += 1;

        if (canMove(newPos, currentPiece.rotation))
        {
            currentPiece.position = newPos;
        }
        else
        {
            lockCurrentPiece();
            checkForLineClears();
            isLocking = true;
            isLockFlashing = true;
            lockFlashTimer = 0.0f;
        }

        fallTimer -= getLevelFallInterval();
    }

    // Update ghost piece every frame so it stays accurate
    updateGhostPiece();
}

// =========================================================================
// buildRenderFrame() — always builds board cells; adds overlay data for
// non-Playing phases so the Renderer can draw the appropriate overlay.
render_data::RenderFrame GameState::buildRenderFrame() const
{
    render_data::RenderFrame frame;
    frame.phase = m_phase;

    // Optimization: avoid unnecessary internal buffer reallocations
    frame.cells.reserve(4 + 4 + config::BOARD_WIDTH * config::BOARD_HEIGHT);

    // --- GHOST PIECE ---
    if (config::showGhost)
    {
        auto blocks{ getTetrominoBlocks(ghostPiece.type, ghostPiece.rotation) };
        for (const auto& relPos : blocks)
        {
            render_data::CellRenderData cell;

            cell.gridPos.x = ghostPiece.position.x + relPos.x;
            cell.gridPos.y = ghostPiece.position.y + relPos.y;
            cell.scale = 1.0f; // normal scale
            cell.tint = glm::vec3(0.75f); // light gray for ghost
            cell.texture = render_data::TextureID::Ghost;

            frame.cells.emplace_back(cell); // add ghost cell to frame
        }
    }

    // --- FALLING TETROMINO ---
    {
        auto blocks{ getTetrominoBlocks(currentPiece.type, currentPiece.rotation) };
        for (const auto& relPos : blocks)
        {
            render_data::CellRenderData cell;

            // Position relative to board origin
            cell.gridPos.x = currentPiece.position.x + relPos.x;
            cell.gridPos.y = currentPiece.position.y + relPos.y;
            cell.scale = 1.0f; // normal scale
            cell.tint = getColorRGB(currentPiece.color);
            cell.texture = render_data::TextureID::Detail;

            frame.cells.emplace_back(cell); // add falling cell to frame
        }
    }

    // --- LOCKED BLOCKS ---
    for (int y{0}; y < config::BOARD_HEIGHT; ++y)
    {
        for (int x{0}; x < config::BOARD_WIDTH; ++x)
        {
            if (board[y][x] == Color::black) continue;

            render_data::CellRenderData cell;

            cell.gridPos.x = x;
            cell.gridPos.y = y;
            cell.texture = render_data::TextureID::Detail;

            // values to calculate
            glm::vec3 tint{ getColorRGB(board[y][x]) };
            float scale{ 1.0f };

            // LOCK FLASH (just landed piece)
            if (isLockFlashing && lockFlashTimer <= LOCK_FLASH_DURATION)
            {
                auto it{ std::find(justLockedCells.begin(),justLockedCells.end(), glm::ivec2(x,y)) };
                if (it != justLockedCells.end())
                {
                    float flash{ std::sin(lockFlashTimer * 40.0f) * 0.5f + 0.5f }; // fast blink
                    tint = glm::vec3(1.0f) * (1.0f + flash * 0.6f); // bright white flash
                }
            }

            // LINE CLEAR FLASH + SHRINK
            if (isClearing)
            {
                auto it{ std::find(clearingRows.begin(), clearingRows.end(), y) };
                if (it != clearingRows.end())
                {
                    float progress{ clearAnimationTimer / CLEAR_ANIMATION_DURATION };
                    float flash{ std::sin(clearAnimationTimer * 35.0f) * 0.5f + 0.5f };

                    // Strong white flash that fades
                    tint = glm::vec3(1.0f) * (1.0f - progress * 0.3f + flash * 0.5f);

                    // Shrink effect (100% → ~40% size)
                    scale = 1.0f - (progress * 0.6f);
                }
            }

            // set calculated values
            cell.scale = scale;
            cell.tint = tint;

            frame.cells.emplace_back(cell); // add locked cell to frame
        }
    }

    // --- HUD ---
    frame.ui.score = m_score;
    frame.ui.level = m_level;

    // NEXT PIECE TETROMINO
    {
        auto nextBlocks{ getTetrominoBlocks(nextPiece.type, nextPiece.rotation) };
        for (const auto& relPos : nextBlocks)
        {
            render_data::CellRenderData cell;

            // Position relative to board origin
            cell.gridPos.x = nextPiece.position.x + relPos.x;
            cell.gridPos.y = nextPiece.position.y + relPos.y;
            cell.scale = 1.0f; // normal scale
            cell.tint = getColorRGB(nextPiece.color);
            cell.texture = render_data::TextureID::Detail;

            frame.ui.nextPieceCells.emplace_back(cell); // add next piece cells to frame
        }
    }

    // HOLD PIECE TETROMINO
    frame.ui.hasHold = m_hasHold;

    if (m_hasHold)
    {
        auto holdBlocks{ getTetrominoBlocks(holdPiece.type, holdPiece.rotation) };
        for (const auto& relPos : holdBlocks)
        {
            render_data::CellRenderData cell;
            cell.gridPos.x = holdPiece.position.x + relPos.x;
            cell.gridPos.y = holdPiece.position.y + relPos.y;
            cell.scale     = 1.0f; // normal scale
            cell.tint      = m_canHold
                             ? getColorRGB(holdPiece.color)
                             : getColorRGB(holdPiece.color) * 0.45f; // dim when unavailable
            cell.texture   = render_data::TextureID::Detail;

            frame.ui.holdPieceCells.emplace_back(cell);
        }
    }

    // --- Overlay data (phase-dependent) ---
    // Leaderboard and gameHasStarted are always populated so the pause menu
    // can display them without knowing which sub-phase we came from.
    frame.overlay.leaderboard    = m_leaderboard;
    frame.overlay.newEntryIndex  = m_newEntryIndex;
    frame.overlay.gameHasStarted = m_gameHasStarted;

    if (m_phase == GamePhase::NameEntry)
    {
        frame.overlay.nameBuffer = m_nameBuffer;
        frame.overlay.finalScore = m_score;
        frame.overlay.finalLevel = m_level;
    }

    return frame;
}

// =========================================================================
// handleInput() — dispatches to the correct phase handler each frame
void GameState::handleInput(GLFWwindow* window, float deltaTime)
{
    switch (m_phase)
    {
    case GamePhase::Playing:
        handlePlayingInput(window, deltaTime);
        break;
    case GamePhase::NameEntry:
        handleNameEntryInput(window);
        break;
    case GamePhase::Paused:
        handlePausedInput(window);
        break;
    default:
        break;
    }
}

// =========================================================================
// Phase-specific input handlers

void GameState::handlePlayingInput(GLFWwindow* window, float deltaTime)
{
    // --- Escape: pause ---
    bool escapePressed{ glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS };
    if (escapePressed && !lastEscapePressed)
        m_phase = GamePhase::Paused;
    lastEscapePressed = escapePressed;

    // --- Clockwise Rotation (single press with debounce) ---
    bool upPressed{ glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS };
    if (upPressed && !lastUpPressed)
    {
        int newRot{ (currentPiece.rotation + 1) % 4 };
        if (canMove(currentPiece.position, newRot))
            currentPiece.rotation = newRot;
    }
    lastUpPressed = upPressed;

    // --- Left (Instant first move + DAS) ---
    bool leftPressed{ glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS };
    if (leftPressed)
    {
        // Instant move on the very first press
        if (!lastLeftPressed)
        {
            glm::ivec2 newPos = currentPiece.position;
            newPos.x -= 1;
            if (canMove(newPos, currentPiece.rotation))
                currentPiece.position = newPos;

            leftRepeatTimer = 0.0f;   // start counting for auto-repeat
        }
        else
        {
            // Auto-repeat after delay
            leftRepeatTimer += deltaTime;
            if (leftRepeatTimer >= DAS_DELAY)
            {
                glm::ivec2 newPos = currentPiece.position;
                newPos.x -= 1;
                if (canMove(newPos, currentPiece.rotation))
                    currentPiece.position = newPos;

                leftRepeatTimer = DAS_DELAY - DAS_REPEAT;  // maintain smooth repeat rate
            }
        }
    }
    else
    {
        leftRepeatTimer = 0.0f;
    }
    lastLeftPressed = leftPressed;

    // --- Right (Instant first move + DAS) ---
    bool rightPressed = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    if (rightPressed)
    {
        if (!lastRightPressed)
        {
            glm::ivec2 newPos = currentPiece.position;
            newPos.x += 1;
            if (canMove(newPos, currentPiece.rotation))
                currentPiece.position = newPos;

            rightRepeatTimer = 0.0f;
        }
        else
        {
            rightRepeatTimer += deltaTime;
            if (rightRepeatTimer >= DAS_DELAY)
            {
                glm::ivec2 newPos = currentPiece.position;
                newPos.x += 1;
                if (canMove(newPos, currentPiece.rotation))
                    currentPiece.position = newPos;

                rightRepeatTimer = DAS_DELAY - DAS_REPEAT;
            }
        }
    }
    else
    {
        rightRepeatTimer = 0.0f;
    }
    lastRightPressed = rightPressed;

    // --- Soft Drop (continuous while held) ---
    bool downPressed{ glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS };
    dropMultiplier = downPressed ? 20.0f : 1.0f; // 20x faster

    // --- HOLD (store current piece in hold bag) ---
    bool holdPressed{ glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS
                   || glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS };
    if (holdPressed && !lastHoldPressed)
        holdCurrentPiece();
    lastHoldPressed = holdPressed;

}

void GameState::handleNameEntryInput(GLFWwindow* window)
{
    // --- Backspace: delete last character ---
    bool backspacePressed{ glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS };
    if (backspacePressed && !lastBackspacePressed)
    {
        if (!m_nameBuffer.empty())
            m_nameBuffer.pop_back();
    }
    lastBackspacePressed = backspacePressed;

    // --- Enter: confirm name (even if empty - defaults to "AAA") ---
    bool enterPressed{ glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS };
    if (enterPressed && !lastEnterPressed)
        confirmName();
    lastEnterPressed = enterPressed;

    // Note: actual character typing is handled via glfwSetCharCallback in main.cpp.
    // GameState exposes appendNameChar() for that callback to call.
}

void GameState::handlePausedInput(GLFWwindow* window)
{
    // Keyboard shortcut: Enter or Space starts / resumes the game
    bool startPressed{ glfwGetKey(window, GLFW_KEY_ENTER)  == GLFW_PRESS
                    || glfwGetKey(window, GLFW_KEY_SPACE)  == GLFW_PRESS };
    if (startPressed && !lastEnterPressed)
        startOrResume();
    lastEnterPressed = startPressed;

    // Keyboard shortcut: Q quits the application
    bool quitPressed{ glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS };
    if (quitPressed && !lastQuitPressed)
        glfwSetWindowShouldClose(window, true);
    lastQuitPressed = quitPressed;

}

void GameState::startOrResume()
{
    if (!m_gameHasStarted)
    {
        // First ever press - board is already set up from constructor, just start
        m_gameHasStarted = true;
    }
    // Either way, just unpause - board state is preserved for resume
    m_phase = GamePhase::Playing;
}

void GameState::quitToMenu()
{
    resetBoard();
    spawnNewPiece();
    m_gameHasStarted = false;
    m_newEntryIndex  = -1;
    m_phase          = GamePhase::Paused;
}

// =========================== Game-over flow ===========================

void GameState::triggerGameOver()
{
    #ifndef NDEBUG
    std::cout << "GAME OVER - score: " << m_score << '\n';
    #endif // NDEBUG

    if (isHighScore(m_score, m_leaderboard))
    {
        m_nameBuffer.clear();
        m_phase = GamePhase::NameEntry;
        // Board and scoring are NOT reset yet - the overlay still displays them
    }
    else
    {
        // Score doesn't qualify - go straight back to pause menu
        m_newEntryIndex  = -1;
        m_gameHasStarted = false;
        resetBoard();
        spawnNewPiece();
        m_phase = GamePhase::Paused;
    }
}

void GameState::confirmName()
{
    if (m_nameBuffer.empty())
        m_nameBuffer = "AAA"; // sensible default if player hits Enter immediately

    HighScore entry{ m_nameBuffer, m_score, m_level };
    insertHighScore(entry, m_leaderboard);
    saveHighScores(m_leaderboard);

    // Find the index of the newly inserted entry so the Renderer can highlight it
    m_newEntryIndex = -1;
    for (int i{ 0 }; i < static_cast<int>(m_leaderboard.size()); ++i)
    {
        if (m_leaderboard[i].name  == entry.name &&
            m_leaderboard[i].score == entry.score &&
            m_leaderboard[i].level == entry.level)
        {
            m_newEntryIndex = i;
            break;
        }
    }

    resetBoard();
    spawnNewPiece();
    m_gameHasStarted = false;
    m_phase = GamePhase::Paused;
}

// ======= Board / scoring reset (does NOT touch the leaderboard) =======
void GameState::resetBoard()
{
    std::fill(&board[0][0],
              &board[0][0] + (config::BOARD_HEIGHT * config::BOARD_WIDTH),
              Color::black);
    m_score        = 0;
    m_level        = 1;
    m_linesCleared = 0;
    fallTimer      = 0.0f;
    dropMultiplier = 1.0f;
    isLocking      = false;
    isLockFlashing = false;
    isClearing     = false;
    clearingRows.clear();
    justLockedCells.clear();
    m_hasHold = false;
    m_canHold = true;
    holdPiece = Tetromino{}; // zero-initialize back to defaults
}

// =========================================================================
// appendNameChar() — called from the GLFW char callback in main.cpp
void GameState::appendNameChar( unsigned int codepoint )
{
    if (m_phase != GamePhase::NameEntry)
        return;

    // Accept only printable ASCII, respect max length
    if (codepoint >= 32 && codepoint <= 126
        && static_cast<int>(m_nameBuffer.size()) < MAX_NAME_LENGTH)
    {
        m_nameBuffer += static_cast<char>(codepoint);
    }
}

// Check if the piece at new position/rotation would be valid
bool GameState::canMove(const glm::ivec2& newPosition, int newRotation) const
{
    auto blocks{ getTetrominoBlocks(currentPiece.type, newRotation) };

    for (const auto& rel : blocks)
    {
        glm::ivec2 cellPos{ newPosition + rel };

        // Wall/Floor check
        if (cellPos.x < 0 || cellPos.x >= config::BOARD_WIDTH || cellPos.y >= config::BOARD_HEIGHT)
            return false; // y < 0 is allowed (spawn area)

        // Block collision check
        if (cellPos.y >= 0 && board[cellPos.y][cellPos.x] != Color::black)
            return false;
    }

    return true;
}

void GameState::lockCurrentPiece()
{
    justLockedCells.clear();
    auto blocks{ getTetrominoBlocks(currentPiece.type, currentPiece.rotation) };
    for (const auto& rel : blocks)
    {
        glm::ivec2 cell{ currentPiece.position + rel };
        if (cell.y >= 0 && cell.y < config::BOARD_HEIGHT && cell.x >= 0 && cell.x < config::BOARD_WIDTH)
        {
            board[cell.y][cell.x] = currentPiece.color; // place color
            justLockedCells.push_back(cell); // keep track of last cells locked
        }
    }

    m_canHold = true;
}

void GameState::holdCurrentPiece()
{
    if (!m_canHold) return;

    if (isLocking || isClearing) return;

    if (!m_hasHold)
    {
        // First hold: stash current, spawn next
        holdPiece.type     = currentPiece.type;
        holdPiece.color    = currentPiece.color;
        holdPiece.rotation = 0;

        int holdX{ 0 }; // Conditional position offset
        if (holdPiece.type == TetrominoType::I || holdPiece.type == TetrominoType::O)
            holdX = -1;

        holdPiece.position = glm::ivec2(holdX, 0);
        m_hasHold          = true;
        spawnNewPiece();
    }
    else
    {
        // Swap current ↔ hold
        TetrominoType swappedType  = holdPiece.type;
        Color         swappedColor = holdPiece.color;

        holdPiece.type     = currentPiece.type;
        holdPiece.color    = currentPiece.color;
        holdPiece.rotation = 0;

        int holdX{ 0 }; // Conditional position offset
        if (holdPiece.type == TetrominoType::I || holdPiece.type == TetrominoType::O)
            holdX = -1;
        holdPiece.position = glm::ivec2(holdX, 0);

        currentPiece.type     = swappedType;
        currentPiece.color    = swappedColor;
        currentPiece.rotation = 0;

        // Re-run spawn positioning logic for the swapped-in piece
        int spawnX{ config::BOARD_WIDTH / 2 };
        if (currentPiece.type == TetrominoType::I || currentPiece.type == TetrominoType::O)
            spawnX -= 1;
        currentPiece.position = glm::ivec2(spawnX, 0);

        fallTimer = 0.0f;
        updateGhostPiece();
    }

    m_canHold = false; // block hold until next lock
}

void GameState::checkForLineClears()
{
    clearingRows.clear();

    // Scan from bottom to top and collect full rows
    for (int y{ config::BOARD_HEIGHT - 1 }; y >= 0; --y)    // bottom to top
    {
        bool rowIsFull{ true };

        for (int x{ 0 }; x < config::BOARD_WIDTH; ++x)
        {
            if (board[y][x] == Color::black)
            {
                rowIsFull = false;
                break;
            }
        }

        if (rowIsFull)
            clearingRows.push_back(y); // add full row to the list
    }

    if (!clearingRows.empty())
    {
        // Scoring & Level
        constexpr int lineScores[]{ 0, 100, 300, 500, 800 };
        int linesCleared{ static_cast<int>(clearingRows.size()) };
        m_linesCleared += linesCleared;
        m_score += lineScores[linesCleared] * m_level;
        m_level = (m_linesCleared / 10) + 1;

        #ifndef NDEBUG
        // debug console output for scoring
        std::cout << "Lines Cleared: " << m_linesCleared << '\n';
        std::cout << "Lvl: " << m_level << '\n';
        std::cout << "Score: " << m_score << '\n';
        #endif // NDEBUG

        isClearing = true;
        clearAnimationTimer = 0.0f;
    }
}

void GameState::spawnNewPiece()
{
    // index (0-6) 7 is out of bounds
    if (m_bagIndex >= 7) refillBag();

    // Define new current piece
    currentPiece.type     = m_bag[m_bagIndex++];
    currentPiece.rotation = 0;
    currentPiece.color    = getTetrominoColor(currentPiece.type);

    // Type-specific x spawn offset to center properly
    int spawnX{ config::BOARD_WIDTH / 2 };

    switch (currentPiece.type)
    {
    case TetrominoType::I:  // widest piece → shift left more
        spawnX -= 1;        // starts at x=3 (blocks at 3,4,5,6)
        break;
    case TetrominoType::O:  // square → slight left shift
        spawnX -= 1;        // starts at x=4 (blocks at 4,5)
        break;
    default: // T, S, Z, J, L — all fit well at x=5 (blocks span ~3–4 columns)
        // spawnX -= 1;  // optional, depending on your visual preference
        break;
    }

    currentPiece.position = {spawnX, 0};  // y=0 → top spawn row

    fallTimer = 0.0f;   // reset for consistent fall ticks

    // refill bag if currentPiece was the last one,
    // so nextPiece can always safely peek at m_bagIndex
    if (m_bagIndex >= 7) refillBag();

    // nextPiece is always the piece after currentPiece — never changes
    // until spawnNewPiece() is called again for a genuine new spawn
    nextPiece.type        = m_bag[m_bagIndex];
    nextPiece.rotation    = 0;
    nextPiece.color       = getTetrominoColor(nextPiece.type);

    // Type-specific x spawn offset to center properly
    int nextX{ 0 };
    if (nextPiece.type == TetrominoType::I || nextPiece.type == TetrominoType::O)
        nextX = -1;

    nextPiece.position    = glm::ivec2(nextX,0);

    updateGhostPiece();

    // Game-over detection
    if (!canMove(currentPiece.position, currentPiece.rotation))
        triggerGameOver();
}

void GameState::refillBag()
{
    m_bag = { TetrominoType::I, TetrominoType::O, TetrominoType::T,
              TetrominoType::S, TetrominoType::Z, TetrominoType::J,
              TetrominoType::L };

    // Fisher-Yates shuffle
    for (int i{ 6 }; i > 0; --i)
    {
        std::uniform_int_distribution<int> dist(0, i);
        std::swap(m_bag[i], m_bag[dist(m_gen)]);
    }

    m_bagIndex = 0;
}

void GameState::updateGhostPiece()
{
    ghostPiece = currentPiece;

    // Drop the ghost straight down until it collides with something
    while (canMove(ghostPiece.position + glm::ivec2(0,1), ghostPiece.rotation))
    {
        ghostPiece.position.y += 1;
    }
}

float GameState::getLevelFallInterval() const
{
    return std::max(0.05f, 1.0f - (m_level - 1) * 0.083f);
}
