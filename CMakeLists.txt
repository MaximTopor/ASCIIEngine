cmake_minimum_required(VERSION 3.20)

project(ASCIIEngine LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GLFW
include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

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
)

# Project include directories
target_include_directories(ASCIIEngine PRIVATE
        include
)

# Project libraries
target_link_libraries(ASCIIEngine PRIVATE
        glfw
        glad
        OpenGL::GL
)

# Copy shaders
add_custom_command(
        TARGET ASCIIEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:ASCIIEngine>/shaders"
)