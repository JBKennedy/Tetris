cmake_minimum_required(VERSION 3.20)
project(Tetris LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Paths — ThirdPartyLibs lives two directories above the project root,
# matching your existing layout: ~/ThirdPartyLibs vs ~/CBProjects/Tetris
# =============================================================================
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/../../ThirdPartyLibs")
set(THIRD_PARTY_INCLUDE "${THIRD_PARTY_DIR}/include")
set(THIRD_PARTY_LIB     "${THIRD_PARTY_DIR}/lib")

# =============================================================================
# Sources
# =============================================================================
set(SOURCES
    main.cpp
    glad.c
    stb_image_implementation.cpp
    src/GameState.cpp
    src/HighScore.cpp
    src/Renderer.cpp
    src/Tetromino.cpp
)

add_executable(Tetris ${SOURCES})

# =============================================================================
# Include directories
# =============================================================================
target_include_directories(Tetris PRIVATE
    include                   # Color.h, GameState.h, etc.
    ${THIRD_PARTY_INCLUDE}    # glad/, GLFW/, glm/, stb/, shader_class/, etc.
)

# =============================================================================
# Compiler warnings & options
# =============================================================================
target_compile_options(Tetris PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
    $<$<CXX_COMPILER_ID:MSVC>:/W3>
)

# =============================================================================
# Linking — platform-specific system libraries
# =============================================================================
if(WIN32)
    target_link_libraries(Tetris PRIVATE
        "${THIRD_PARTY_DIR}/lib-windows/libglfw3.a"
        opengl32
        gdi32
        user32
        winmm
    )
elseif(APPLE)
    target_link_libraries(Tetris PRIVATE
        "${THIRD_PARTY_LIB}/libglfw3.a"
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    # Linux
    target_link_libraries(Tetris PRIVATE
        "${THIRD_PARTY_LIB}/libglfw3.a"
        GL
        dl
        pthread
    )
endif()

target_compile_definitions(Tetris PRIVATE ASSETS_NEXT_TO_EXE)

# =============================================================================
# Build type defaults
# =============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

target_compile_options(Tetris PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O2>
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-g>
)

# =============================================================================
# Copy assets (shaders + textures) next to the executable after build
# so that getExeDir() / working-directory logic in main.cpp works correctly.
# =============================================================================
add_custom_command(TARGET Tetris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:Tetris>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/textures"
        "$<TARGET_FILE_DIR:Tetris>/textures"
    COMMENT "Copying shaders and textures to output directory"
)
