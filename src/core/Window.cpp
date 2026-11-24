#include "core/Window.hpp"

#include <iostream>
#include <stdexcept>

Window::Window(
    int width,
    int height,
    const std::string& title
)
{
    if (!glfwInit())
    {
        throw std::runtime_error(
            "Failed to initialize GLFW"
        );
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    window = glfwCreateWindow(
        width,
        height,
        title.c_str(),
        nullptr,
        nullptr
    );

    if (!window)
    {
        glfwTerminate();

        throw std::runtime_error(
            "Failed to create window"
        );
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        throw std::runtime_error(
            "Failed to initialize GLAD"
        );
    }

    int framebufferWidth;
    int framebufferHeight;

    glfwGetFramebufferSize(
        window,
        &framebufferWidth,
        &framebufferHeight
    );

    glViewport(
        0,
        0,
        framebufferWidth,
        framebufferHeight
    );

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );

    glfwSwapInterval(1);

    std::cout << "OpenGL: "
              << glGetString(GL_VERSION) << '\n';

    std::cout << "GPU: "
              << glGetString(GL_RENDERER) << '\n';
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::framebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height
)
{
    glViewport(0, 0, width, height);
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::close()
{
    glfwSetWindowShouldClose(window, true);
}

void Window::swapBuffers() const
{
    glfwSwapBuffers(window);
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) const
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

void Window::getFramebufferSize(
    int& width,
    int& height
) const
{
    glfwGetFramebufferSize(
        window,
        &width,
        &height
    );
}

void Window::setTitle(
    const std::string& title
) const
{
    glfwSetWindowTitle(
        window,
        title.c_str()
    );
}