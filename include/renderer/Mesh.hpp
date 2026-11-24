#pragma once

#include <glad/gl.h>
#include <vector>

struct Vertex
{
    float x, y, z;
    float u, v;
};

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei indexCount = 0;

public:
    Mesh(
        const std::vector<Vertex>& vertices,
        const std::vector<GLuint>& indices
    );

    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void bind() const;
    void unbind() const;

    GLsizei getIndexCount() const;
};