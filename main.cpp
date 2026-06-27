#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>

// for platform dependent working directory resolution
#ifdef _WIN32
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

#include "GameState.h"
#include "Renderer.h"

// ==================== GLOBALS / CONSTANTS ====================

// ==================== FORWARD DECLARATIONS ====================
GLFWwindow* createWindow();
std::filesystem::path getExeDir();
float calculateDeltaTime();
void handleSystemInput(GLFWwindow* window);

// ============================ MAIN ============================
int main(void)
{
    // 1. Window Creation and OpenGL Initialization
    GLFWwindow* window{ createWindow() };
    if (!window) return -1;

    // Initialize GLAD: load all OpenGL function pointers
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << '\n';
        glfwTerminate();
        return -1;
    }

    // Enable Transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFuncSeparate prevents the desktop compositor (Linux/X11/Wayland)
    // from treating the framebuffer alpha as window transparency against the desktop
    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // 2. Set Working Directory
    #ifdef ASSETS_NEXT_TO_EXE  // for CMake
        #ifdef __APPLE__
            // Assets are in Contents/Resources/, one level up and across from Contents/MacOS/
            std::filesystem::current_path(getExeDir() / ".." / "Resources");
        #else
            std::filesystem::current_path(getExeDir());
        #endif
    #else                      // for Code::Blocks
        // Code::Blocks puts the exe in bin/Debug or bin/Release/
        // relative to the project root where shaders/ and textures/ live
        std::filesystem::current_path(getExeDir() / ".." / "..");
    #endif

    // 3. Initialize Renderer
    Renderer renderer{ window };

    // 4. Initialize Game State
    GameState game;

    // 5. Callbacks
    struct AppContext { GameState* game; Renderer* renderer; };
    static AppContext ctx{ &game, &renderer };
    glfwSetWindowUserPointer(window, &ctx);

    glfwSetCharCallback(window, [](GLFWwindow* w, unsigned int codepoint)
    {
        auto* c{ static_cast<AppContext*>(glfwGetWindowUserPointer(w)) };
        if (c) c->game->appendNameChar(codepoint);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y)
    {
        auto* c{ static_cast<AppContext*>(glfwGetWindowUserPointer(w)) };
        if (c) c->renderer->checkHover(x, y);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int /*mods*/)
    {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
        auto* c{ static_cast<AppContext*>(glfwGetWindowUserPointer(w)) };
        if (!c) return;

        double mx, my;
        glfwGetCursorPos(w, &mx, &my);

        switch (c->renderer->checkClick(mx, my))
        {
        case MenuButton::Play: c->game->startOrResume(); break;
        case MenuButton::Quit: glfwSetWindowShouldClose(w, true);    break;
        default: break;
        }
    });

    // 6. Main Game Loop
    while(!glfwWindowShouldClose(window))
    {
        // Input & Update
        float deltaTime{ calculateDeltaTime() };
        handleSystemInput(window);
        game.handleInput(window, deltaTime);
        game.update(window, deltaTime);

        // Render
        render_data::RenderFrame frame{ game.buildRenderFrame() };
        renderer.draw(frame, deltaTime);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // 7. Cleanup
    glfwTerminate();

	return 0;
}
// ========================= Helper Functions =========================
// Creates and configures the GLFW window
GLFWwindow* createWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --- Borderless windowed fullscreen ---
    GLFWmonitor* monitor{ glfwGetPrimaryMonitor() };
    const GLFWvidmode* mode{ glfwGetVideoMode(monitor) };

    // Match desktop's existing video mode so OS skips a mode-switch (no black flash)
    glfwWindowHint(GLFW_RED_BITS,     mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS,   mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS,    mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    // macOS: exclusive fullscreen swallows trackpad/mouse input via the OS event system.
    // Borderless windowed fullscreen is the correct approach on MacOS for OpenGL apps.
    // It's also generally a better choice for windows and linux.
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    GLFWwindow* window{ glfwCreateWindow(mode->width, mode->height,
                                         "Tetris", nullptr, nullptr) };
    if (window)
        glfwSetWindowPos(window, 0, 0); // anchor to top-left of screen

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << '\n';
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    return window;
}

// Resolve paths relative to the executable, for cross-platform compatability
std::filesystem::path getExeDir()
{
#if defined(_WIN32)
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#elif defined(__APPLE__)
    char buf[1024];
    uint32_t size{ sizeof(buf) };
    _NSGetExecutablePath(buf, &size);
    return std::filesystem::canonical(buf).parent_path();
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
}

// Calculates deltaTime and updates lastFrame
float calculateDeltaTime()
{
    static float lastFrame{ 0.0f };
    float currentFrame{ static_cast<float>(glfwGetTime()) };
    float deltaTime{ currentFrame - lastFrame };
    lastFrame = currentFrame;
    return deltaTime;
}

// process system level input (only esc to close for now)
void handleSystemInput(GLFWwindow* window)
{
    /* Remove this later
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true); */
}
