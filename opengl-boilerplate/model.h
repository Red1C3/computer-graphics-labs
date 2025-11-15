#pragma once
#include<vector>
#include<glm/glm.hpp>
#include<GL/glew.h>

using namespace glm;
using namespace std;


class Model {
public:
    Model(vector<vec3> postions, vector<vec2> uvs);
    void draw();
private:
    GLuint VAO;
    unsigned long verticesCount;
};