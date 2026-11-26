#pragma once

#include <glad/gl.h>

class Texture
{
private:
    GLuint textureID = 0;

    int width;
    int height;

public:
    Texture(
        int width,
        int height,
        const unsigned char* pixels,
        int mipLevels = 0
    );

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(GLuint slot = 0) const;
    void unbind(GLuint slot = 0) const;

    GLuint getID() const;

    int getWidth() const;
    int getHeight() const;
};
