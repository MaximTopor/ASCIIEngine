#include "renderer/Shader.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace
{
    struct Context
    {
        GLFWwindow* window = nullptr;
        Context()
        {
            if (!glfwInit()) return;
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            window = glfwCreateWindow(32,32,"Glyph lighting test",nullptr,nullptr);
            if (window)
            {
                glfwMakeContextCurrent(window);
                if (!gladLoadGL(glfwGetProcAddress))
                {
                    glfwDestroyWindow(window);
                    window = nullptr;
                }
            }
        }
        ~Context()
        {
            if (window) glfwDestroyWindow(window);
            glfwTerminate();
        }
    };

    void require(bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }

    std::uint32_t normal(float x,float y,float z)
    {
        const auto q = [](float v) { return static_cast<std::uint32_t>(std::lround((v+1)*511.5f)); };
        return q(x) | (q(y)<<10) | (q(z)<<20) | (1u<<30);
    }

    std::uint32_t material(unsigned roughness,unsigned reflectivity)
    {
        return roughness | (reflectivity<<8) | (255u<<16);
    }

    void verifyShader()
    {
        Shader shader("shaders/glyph.vert","shaders/glyph.frag");
        shader.use();
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.getID(),GL_LINK_STATUS,&linked);
        require(linked == GL_TRUE,"glyph shader link failed");

        GLuint vao = 0, vertices = 0;
        std::array<GLuint,2> textures{};
        glCreateVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glCreateBuffers(1,&vertices);
        constexpr float positions[]{-0.5f,-0.5f,0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f};
        glNamedBufferStorage(vertices,sizeof(positions),positions,0);
        glVertexArrayVertexBuffer(vao,0,vertices,0,2*sizeof(float));
        glVertexArrayAttribFormat(vao,0,2,GL_FLOAT,GL_FALSE,0);
        glEnableVertexArrayAttrib(vao,0);
        glVertexAttrib2f(3,30,30);
        glVertexAttrib4f(4,0,0,1,1);
        glVertexAttrib3f(5,0.4f,0.4f,0.4f);
        glVertexAttrib4f(7,0,1,0.5f,0.5f);
        glVertexAttrib4f(9,-15,-15,0,1);
        glVertexAttribI2ui(10,normal(1,0,0),material(230,0));

        glCreateTextures(GL_TEXTURE_2D,2,textures.data());
        for (GLuint texture : textures)
        {
            glTextureStorage2D(texture,1,GL_RGBA8,1,1);
            glTextureParameteri(texture,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTextureParameteri(texture,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        }
        const auto texel = [](GLuint texture, std::array<unsigned char,4> rgba)
        {
            glTextureSubImage2D(texture,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        };
        texel(textures[0],{255,255,255,255});
        texel(textures[1],{40,60,80,255});
        glBindTextureUnit(0,textures[0]);
        glBindTextureUnit(1,textures[1]);
        glViewport(0,0,32,32);
        glDisable(GL_DITHER);
        glClearColor(0,0,0,0);

        shader.setInt("glyphTexture",0);
        shader.setInt("uLightmap",1);
        shader.setInt("uCoveragePass",3);
        shader.setVec2("uScreenSize",32,32);
        shader.setVec4("uCameraBasis",1,0,1,0);
        shader.setFloat("uCameraZoom",1);
        shader.setVec3("uSurfaceSky",0.2f,0.2f,0.2f);
        shader.setVec3("uSurfaceGround",0.2f,0.2f,0.2f);
        shader.setInt("uSurfacePointCount",1);
        shader.setVec3("uSurfaceRadiance[0]",1,1,1);
        shader.setVec3("uSunDirection",1,0,0);
        shader.setVec3("uMoonDirection",0,0,1);

        const auto red = [&]()
        {
            std::array<unsigned char,4> pixel{};
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLE_STRIP,0,4);
            glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
            return static_cast<int>(pixel[0]);
        };
        shader.setVec4("uSurfacePoint[0]",20,0,0,100);
        const int front = red();
        shader.setVec4("uSurfacePoint[0]",-20,0,0,100);
        const int back = red();
        require(front > back+50,"moving the torch did not change the illuminated side");
        shader.setVec4("uSurfacePoint[0]",20,0,0,100);
        texel(textures[1],{240,60,80,0});
        require(std::abs(red()-back)<=1,"point visibility did not suppress direct light");
        texel(textures[1],{240,60,80,255});
        require(std::abs(red()-front)<=1,"legacy RGB lightmap leaked into surface lighting");
        shader.setInt("uSurfacePointCount",0);
        shader.setVec3("uSunRadiance",0.5f,0.5f,0.5f);
        require(red() > back+40,"directional diffuse did not illuminate front normal");
        shader.setVec3("uSunDirection",-1,0,0);
        require(std::abs(red()-back)<=1,"directional diffuse lit the back normal");
        shader.setInt("uLightingDebug",6);
        shader.setVec3("uSunDirection",0,0,1);
        glVertexAttribI2ui(10,normal(0,0,1),material(64,204));
        const int steel = red();
        glVertexAttribI2ui(10,normal(0,0,1),material(230,204));
        require(steel > red()+30,"roughness did not weaken the specular peak");
        glVertexAttribI2ui(10,normal(0,0,1),material(64,10));
        require(steel > red()+30,"reflectivity did not affect specular response");
        shader.setInt("uLightingDebug",2);
        require(std::abs(red()-128)<=2,"normal visualization did not use world-space normal");
        texel(textures[0],{60,60,60,255});
        require(red()==0,"surface lighting changed alpha cutout");
        require(glGetError()==GL_NO_ERROR,"OpenGL error during shader assertions");

        glDeleteTextures(2,textures.data());
        glDeleteBuffers(1,&vertices);
        glDeleteVertexArrays(1,&vao);
    }
}

int main()
{
    Context context;
    if (!context.window)
    {
        std::cerr << "No OpenGL 4.5 context available\n";
        return 77;
    }
    try
    {
        verifyShader();
        std::cout << "Point direction, visibility, directional diffuse, material response and cutout passed.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
