#pragma once

#include <glad/gl.h>
#include <string>

class Shader
{
private:
    GLuint programID = 0;

    static std::string readFile(
        const std::string& path
    );

    static GLuint compileShader(
        GLenum type,
        const std::string& source
    );

public:
    Shader(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    GLuint getID() const;

    void setInt(const std::string& name, int value) const;
};