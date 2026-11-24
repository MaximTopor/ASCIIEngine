#include "C:\Users\MAX\CLionProjects\ASCIIEngine\include\renderer\Shader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string Shader::readFile(
    const std::string& path
)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open shader: " + path
        );
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

GLuint Shader::compileShader(
    GLenum type,
    const std::string& source
)
{
    GLuint shader = glCreateShader(type);

    const char* code = source.c_str();

    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[2048];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            nullptr,
            log
        );

        glDeleteShader(shader);

        throw std::runtime_error(log);
    }

    return shader;
}

Shader::Shader(
    const std::string& vertexPath,
    const std::string& fragmentPath
)
{
    const std::string vertexSource =
        readFile(vertexPath);

    const std::string fragmentSource =
        readFile(fragmentPath);

    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource);

    GLuint fragmentShader;

    try
    {
        fragmentShader = compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource
        );
    }
    catch (...)
    {
        glDeleteShader(vertexShader);
        throw;
    }

    programID = glCreateProgram();

    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);

    glLinkProgram(programID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success;

    glGetProgramiv(
        programID,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char log[2048];

        glGetProgramInfoLog(
            programID,
            sizeof(log),
            nullptr,
            log
        );

        glDeleteProgram(programID);

        throw std::runtime_error(log);
    }
}

Shader::~Shader()
{
    glDeleteProgram(programID);
}

void Shader::use() const
{
    glUseProgram(programID);
}

GLuint Shader::getID() const
{
    return programID;
}

void Shader::setInt(
    const std::string& name,
    int value
) const
{
    glProgramUniform1i(
        programID,
        glGetUniformLocation(programID, name.c_str()),
        value
    );
}
void Shader::setVec2(
    const std::string& name,
    float x, float y
) const
{
    glProgramUniform2f(
        programID,
        glGetUniformLocation(programID, name.c_str()),
        x, y
    );
}

void Shader::setVec3(
    const std::string& name,
    float x, float y, float z
) const
{
    glProgramUniform3f(
        programID,
        glGetUniformLocation(programID, name.c_str()),
        x, y, z
    );
}

void Shader::setVec4(
    const std::string& name,
    float x, float y, float z, float w
) const
{
    glProgramUniform4f(
        programID,
        glGetUniformLocation(programID, name.c_str()),
        x, y, z, w
    );
}
void Shader::setFloat(
    const std::string& name,
    float value
) const
{
    glProgramUniform1f(
        programID,
        glGetUniformLocation(
            programID,
            name.c_str()
        ),
        value
    );
}