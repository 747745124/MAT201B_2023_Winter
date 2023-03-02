#pragma once
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include <glm/glm.hpp>
#include <stb_image.h>
using namespace std;
using namespace al;

GLfloat vertices[] = {
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f};

class SkyBox
{
public:
    SkyBox()
    {
        GLfloat vertices[] = {
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f};

        // create vao and vbo
        // glGenVertexArrays(1, &_vao);
        // glGenBuffers(1, &_vbo);

        // glBindVertexArray(_vao);
        // glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        // glEnableVertexAttribArray(0);
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

        // glBindVertexArray(0);

        // try
        // {
        //     // init texture
        // glGenTextures(1, &_texture);
        //     // -----------------------------------------------
        //     // write your code to generate texture cubemap
        //     glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
        //     int width = 0, height = 0, channels = 0;

        //     for (unsigned int i = 0; i < _paths.size(); i++)
        //     {
        //         unsigned char *data = stbi_load(_paths[i].c_str(), &width, &height, &channels, 0);
        //         if (data)
        //         {
        //             glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        //             stbi_image_free(data);
        //         }
        //         else
        //         {
        //             std::stringstream ss;
        //             ss << "Cubemap texture failed to load at path: " << _paths[i];
        //             stbi_image_free(data);
        //         }
        //     }

        //     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        //     const char *vertCode =
        //         "#version 330 core\n"
        //         "layout(location = 0) in vec3 aPosition;\n"
        //         "out vec3 texCoord;\n"
        //         "uniform mat4 projection;\n"
        //         "uniform mat4 view;\n"
        //         "void main() {\n"
        //         "   texCoord = aPosition;\n"
        //         "   gl_Position = (projection * view * vec4(aPosition, 1.0f)).xyww;\n"
        //         "}\n";

        //     const char *fragCode =
        //         "#version 330 core\n"
        //         "out vec4 color;\n"
        //         "in vec3 texCoord;\n"
        //         "uniform samplerCube cubemap;\n"
        //         "void main() {\n"
        //         "   color = texture(cubemap, texCoord);\n"
        //         "}\n";

        //     _shader.compile(slurp(vertCode), slurp(fragCode));
        // }
        // catch (const std::exception &)
        // {
        //     cleanup();
        //     throw;
        // }

        // GLenum error = glGetError();
        // if (error != GL_NO_ERROR)
        // {
        //     std::stringstream ss;
        //     ss << "skybox creation failure, (code " << error << ")";
        //     cleanup();
        //     throw std::runtime_error(ss.str());
        // }
    };

    ~SkyBox();

    void draw(const glm::mat4 &projection, const glm::mat4 &view);

    string slurp(string fileName)
    {
        fstream file(fileName);
        string returnValue = "";
        while (file.good())
        {
            string line;
            getline(file, line);
            returnValue += line + "\n";
        }
        return returnValue;
    }

    ShaderProgram _shader;
    GLuint _vao;
    GLuint _vbo;
    GLuint _texture;

private:
    const std::vector<std::string> _paths = {"./cubemap/right.jpg", "./cubemap/left.jpg", "./cubemap/top.jpg", "./cubemap/bottom.jpg", "./cubemap/front.jpg", "./cubemap/back.jpg"};
    void cleanup();
};

// SkyBox::SkyBox()

SkyBox::~SkyBox()
{
    cleanup();
}

void SkyBox::draw(const glm::mat4 &projection, const glm::mat4 &view)
{
    // -----------------------------------------------
    // write your code here

    //  glDepthFunc(GL_LEQUAL);
    //  _shader->use();
    //  _shader->setMat4("projection", projection);
    //  _shader->setMat4("view", glm::mat4(glm::mat3(view)));
    //  _shader->setInt("cubemap", 0);

    // glBindVertexArray(_vao);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);

    // glDrawArrays(GL_TRIANGLES, 0, 36);
    // glBindVertexArray(0);
    // glDepthFunc(GL_LESS); // set depth function back to default

    // -----------------------------------------------
}

void SkyBox::cleanup()
{
    if (_vbo != 0)
    {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0)
    {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}