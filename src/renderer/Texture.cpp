#include "renderer/Texture.hpp"

#include <stdexcept>

Texture::Texture(
    int width,
    int height,
    const unsigned char* pixels,
    int mipLevels
)
    : width(width),
      height(height)
{
    if (width <= 0 || height <= 0 || pixels == nullptr)
    {
        throw std::invalid_argument("Invalid texture data");
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLint previousAlignment;

    glGetIntegerv(
        GL_UNPACK_ALIGNMENT,
        &previousAlignment
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        width,
        height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        previousAlignment
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        mipLevels > 0 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    if (mipLevels > 0)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    glDeleteTextures(1, &textureID);
}

void Texture::bind(GLuint slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind(GLuint slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture::getID() const
{
    return textureID;
}

int Texture::getWidth() const
{
    return width;
}

int Texture::getHeight() const
{
    return height;
}
