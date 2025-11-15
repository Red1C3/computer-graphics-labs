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

    // Load the model file
    // Note that we are triangulating it here with aiProcess_Triangulate flag
    // So that we are sure everything is composed of triangles
    const aiScene* scene = importer.ReadFile(meshPath, aiProcess_Triangulate);

    // We want the first mesh (group of vertices) in the model file
    aiMesh* mesh = scene->mMeshes[0];

    vector<vec3> positions(mesh->mNumFaces * 3);
    vector<vec2> uvs(mesh->mNumFaces * 3);

    // Now we iterate over the faces of the mesh
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        // For each face
        for (int j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
            
            // We obtain the index of the vertex and grab its position and uv data
            auto position = mesh->mVertices[mesh->mFaces[i].mIndices[j]];
            auto uv = mesh->mTextureCoords[0][mesh->mFaces[i].mIndices[j]];

            positions.push_back({ position.x,position.y,position.z });
            uvs.push_back({ uv.x,uv.y });
        }
    }

    return Model(positions, uvs);
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

    vector<vec3> positions = {};

    // First face
    positions.push_back({ 1, 1, 1 });
    positions.push_back({ 1, -1, -1 });
    positions.push_back({ -1, 1, -1 });

    // Second face
    positions.push_back({ 1, 1, 1 });
    positions.push_back({ 1, -1, -1 });
    positions.push_back({ -1, -1, 1 });

    // Third face
    positions.push_back({ 1, -1, -1 });
    positions.push_back({ -1, 1, -1 });
    positions.push_back({ -1, -1, 1 });

    // Fourth face
    positions.push_back({ 1, 1, 1 });
    positions.push_back({ -1, 1, -1 });
    positions.push_back({ -1, -1, 1 });

    vector<vec2> uvs;

    // First face
    uvs.push_back({ 1, 1 });
    uvs.push_back({ 0,0 });
    uvs.push_back({ 0, 1 });

    // Second face
    uvs.push_back({ 1, 1 });
    uvs.push_back({ 1, 0 });
    uvs.push_back({ 0,0 });

    // Third face
    uvs.push_back({ 1, 0 });
    uvs.push_back({ 0,1 });
    uvs.push_back({ 0,0 });

    // Fourth face
    uvs.push_back({ 1, 1 });
    uvs.push_back({ 0, 1 });
    uvs.push_back({ 0,0 });

    Model tetrahedron(positions, uvs);

    // We load this model from file
    // The file path is passed as a parameter
    Model sphere = loadModelFromFile("sphere.glb");

    if (GLenum err = glGetError() != 0) {
        return err;
    }

    // We load two textures into the VRAM
    GLuint texture = generateTexture("texture.png");
    GLuint texture2 = generateTexture("texture2.png");

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

        angle += clock.restart().asSeconds();

        mat4 rotation = rotate(mat4(1.0f), angle, { 0, 1, 0 });
        mat4 translation = translate(mat4(1.0f), { 0, 0, -7 });

        // We first apply the rotation (around the center)
        // Followed by a translation
        mat4 modelMatrix = translation * rotation;

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        // Note that now we ALSO clear the depth values
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set the current texture
        glBindTexture(GL_TEXTURE_2D, texture);
        tetrahedron.draw();

        // We update the model matrix so our models do not overlap
        modelMatrix = translate(mat4(1.0f), { 2,0,-7 });
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        // Switch the current texture as well
        glBindTexture(GL_TEXTURE_2D, texture2);
        sphere.draw();

        window.display();
    }
}