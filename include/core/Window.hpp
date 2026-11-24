#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

class Window
{
private:
    GLFWwindow* window = nullptr;

    static void framebufferSizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );

    double scrollOffset = 0.0;

    static void scrollCallback(
        GLFWwindow* window,
        double xOffset,
        double yOffset
    );

public:
    Window(
        int width,
        int height,
        const std::string& title
    );

    ~Window();
    double consumeScroll();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void close();

    void swapBuffers() const;
    void pollEvents() const;

    bool isKeyPressed(int key) const;

    void getFramebufferSize(
    int& width,
    int& height
    )const;
    void setTitle(const std::string& title) const;
};