cmake_minimum_required(VERSION 3.20)

project(ASCIIEngine LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

# FreeType
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-3
)

FetchContent_MakeAvailable(freetype)

# GLAD
add_library(glad STATIC
        external/glad/src/gl.c
)

target_include_directories(glad PUBLIC
        external/glad/include
)

# OpenGL
find_package(OpenGL REQUIRED)

# Project source files
add_executable(ASCIIEngine
        src/main.cpp

        src/core/Application.cpp
        src/core/Window.cpp

        src/renderer/Shader.cpp
        src/renderer/Mesh.cpp
        src/renderer/Texture.cpp
        src/renderer/Renderer.cpp
        src/renderer/Font.cpp
        src/renderer/FontAtlas.cpp
)

# Project include directories
target_include_directories(ASCIIEngine PRIVATE
        include
)

# Project libraries
target_link_libraries(ASCIIEngine PRIVATE
        glfw
        glad
        freetype
        OpenGL::GL
)

# Copy shaders and assets
add_custom_command(
        TARGET ASCIIEngine POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:ASCIIEngine>/shaders"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:ASCIIEngine>/assets"
)

target_sources(ASCIIEngine PRIVATE
        src/renderer/GlyphRenderer.cpp
)

target_sources(ASCIIEngine PRIVATE
        src/world/World.cpp
        src/entity/Player.cpp
)