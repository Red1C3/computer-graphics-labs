#pragma once
#include<GL/glew.h>


class ShaderProgram {
public:
    ShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath);
    bool getError();
    GLuint getProgram();
private:
    GLuint program;
    bool error = false;
};