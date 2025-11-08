#include <SFML/Window/ContextSettings.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <vector>

#include <GL/glew.h>
#include <SFML/System/Clock.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "model.h"
#include "shader_program.h"

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 800

using namespace std;
using namespace sf;
using namespace glm;

int main() {
    ContextSettings ctxSettings;
    ctxSettings.minorVersion = 3;
    ctxSettings.majorVersion = 3;

    // Note that we need to specify the depth bits in order to use depth
    ctxSettings.depthBits = 24;

    Window window(VideoMode({ WINDOW_WIDTH,WINDOW_HEIGHT }), "Hello Tetrahedron", Style::Default, State::Windowed, ctxSettings);

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

    vector<vec3> colors = {};

    // First face
    for (int i = 0; i < 3; ++i) {
        colors.push_back({ 1, 1, 1 });
    }

    // Second face
    for (int i = 0; i < 3; ++i) {
        colors.push_back({ 1, 0, 0 });
    }

    // Third face
    for (int i = 0; i < 3; ++i) {
        colors.push_back({ 0, 1, 0 });
    }

    // Forth face
    for (int i = 0; i < 3; ++i) {
        colors.push_back({ 0, 0, 1 });
    }

    Model tetrahedron(positions, colors);

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
        mat4 translation = translate(mat4(1.0f), { 0, 0, -5 });

        // We first apply the rotation (around the center)
        // Followed by a translation
        mat4 modelMatrix = translation * rotation;

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        // Note that now we ALSO clear the depth values
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        tetrahedron.draw();

        window.display();
    }
}