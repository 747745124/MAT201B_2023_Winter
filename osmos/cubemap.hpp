#pragma once
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include <glm/glm.hpp>

class SkyBox
{
public:
    SkyBox(const std::vector<std::string> &textureFilenames);

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

private:
    GLuint _vao = 0;
    GLuint _vbo = 0;

    Texture _texture;
    ShaderProgram _shader;

    void cleanup();
};