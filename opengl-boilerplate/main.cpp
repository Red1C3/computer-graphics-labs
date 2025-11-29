#include <SFML/Window/ContextSettings.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <vector>

#include <GL/glew.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "model.h"
#include "shader_program.h"

#define WINDOW_HEIGHT 720
#define WINDOW_WIDTH 1280

using namespace std;
using namespace sf;
using namespace glm;
using namespace Assimp;

GLuint generateTexture(const char* imagePath) {
    // Load image file
    Image img(imagePath);

    // UV space coords is vertically inverted to the image space
    // So we need to flip the image ourselves before loading it into the VRAM
    img.flipVertically();

    GLuint texture;

    glGenTextures(1, &texture);

    // Set as current
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set wrapping modes along u (s) and v (t)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set minimize and maximize filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image data into VRAM
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());

    return texture;
}

Model loadModelFromFile(const char* meshPath) {
    Importer importer;

    const aiScene* scene = importer.ReadFile(meshPath, aiProcess_Triangulate);

    aiMesh* mesh = scene->mMeshes[0];

    vector<vec3> positions(mesh->mNumFaces * 3);
    vector<vec2> uvs(mesh->mNumFaces * 3);
    vector<vec3> normals(mesh->mNumFaces * 3); // Read and parse normals as well

    for (int i = 0; i < mesh->mNumFaces; ++i) {
        for (int j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
            auto position = mesh->mVertices[mesh->mFaces[i].mIndices[j]];
            auto uv = mesh->mTextureCoords[0][mesh->mFaces[i].mIndices[j]];
            auto normal = mesh->mNormals[mesh->mFaces[i].mIndices[j]];

            positions.push_back({ position.x, position.y, position.z });
            uvs.push_back({ uv.x, uv.y });
            normals.push_back({ normal.x, normal.y, normal.z });
        }
    }

    return Model(positions, uvs, normals);
}

int main() {
    ContextSettings ctxSettings;
    ctxSettings.minorVersion = 3;
    ctxSettings.majorVersion = 3;

    // Note that we need to specify the depth bits in order to use depth
    ctxSettings.depthBits = 24;

    Window window(VideoMode({ WINDOW_WIDTH,WINDOW_HEIGHT }), "Hello Textures", Style::Default, State::Windowed, ctxSettings);

    window.setActive(true);

    if (glewInit() != GLEW_OK) {
        return -1;
    }

    // Enable depth
    glEnable(GL_DEPTH_TEST);
    
    // Enable blending and set the blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    Model sphere = loadModelFromFile("sphere.glb");

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    GLuint texture = generateTexture("texture2.png");

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    ShaderProgram shaderProgram("shaders/shader.vert", "shaders/shader.frag");

    if (shaderProgram.getError()) {
        return -1;
    }

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    // Each uniform has a location (think of it as an ID)
    // Before we update the uniform we need to get its location
    GLuint modelMatrixLocation =
        glGetUniformLocation(shaderProgram.getProgram(), "model");
    GLuint viewMatrixLocation =
        glGetUniformLocation(shaderProgram.getProgram(), "view");
    GLuint perspectiveMatrixLocation =
        glGetUniformLocation(shaderProgram.getProgram(), "perspective");

    GLuint transparencyLocation =
        glGetUniformLocation(shaderProgram.getProgram(), "transparent");

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    // Note that we cannot update the uniform without setting the shader program
    // first
    glUseProgram(shaderProgram.getProgram());

    vec3 cameraPosition{ 0, 0, 0 };
    vec3 lookedAt{ 0, 0, -1 };
    vec3 up{ 0, 1, 0 };

    // View matrix controls the camera/observer
    mat4 viewMatrix = lookAt(cameraPosition, lookedAt, up);

    // We send the pointer to the first element of the matrix (notice the [0][0])
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

    float fieldOfView = 45.0f;
    float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float nearPlane = 0.01f;
    float farPlane = 100.0f;

    mat4 perspectiveMatrix =
        perspective(fieldOfView, aspectRatio, nearPlane, farPlane);

    glUniformMatrix4fv(perspectiveMatrixLocation, 1, GL_FALSE,
        &perspectiveMatrix[0][0]);

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    // We'll use this angle to rotate the tetrahedron
    float angle = 0.f;

    Clock clock;

    bool running = true;
    while (running) {
        while (const optional event=window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                running = false;
            }
            else if (const auto* resized=event->getIf<Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);
            }
        }

        // Note that now we ALSO clear the depth values
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        angle += clock.restart().asSeconds();

        mat4 rotation = rotate(mat4(1.0f), angle, { 0, 1, 0 });
        mat4 translation = translate(mat4(1.0f), { 0, 0, -7 });

        // We first apply the rotation (around the center)
        // Followed by a translation
        mat4 modelMatrix = translation * rotation;

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniform1i(transparencyLocation, 0); // Not transparent

        glBindTexture(GL_TEXTURE_2D, texture);
        sphere.draw();

        modelMatrix = translate(mat4(1.0f), { 1, -1, -5 });

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniform1i(transparencyLocation, 1); //Transparent
        sphere.draw();

        window.display();
    }
}