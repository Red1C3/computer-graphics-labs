#include <SFML/Window/ContextSettings.hpp>
#include <vector>
#include<iostream>

#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <glm/glm.hpp>

#include "shaders.h"


using namespace std;
using namespace sf;
using namespace glm;

// Vertex Array Object is like a reference to an object
// It holds another reference to the object's location in VRAM
// As well as how this object's data is going to be sent to the vertex shader
GLuint generateVertexArrayObject(vector<vec3> vertices) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // A buffer is a reference in the VRAM
    GLuint VBO;

    // First we define a buffer
    glGenBuffers(1, &VBO);

    // Then we set it as the active buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Now we are ready to send data to VRAM
    // We send the size of the whole data
    // As well as the data itself
    // Static draw tells the GPU we will not be writing to this buffer more than once
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size(), vertices.data(),
        GL_STATIC_DRAW);

    // We describe how the data is read from the vertex shader
    // Notice how the position is 3 floats so we set the stride (space between
    // each vertex) to 3*sizeof(float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Notice the last parameter
    // We skip the position data so this data starts at the fourth vector
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3 * sizeof(vec3)));

    // Finally we need to set the location 0 as enabled for position
    glEnableVertexAttribArray(0);

    // and location 1 as enabled for color
    glEnableVertexAttribArray(1);

    return VAO;
}

GLuint loadShaders(const char* vertexShaderCode,
    const char* fragmentShaderCode) {
    GLuint program = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // We send the shader's code to the GPU
    glShaderSource(vertexShader, 1, &vertexShaderCode, nullptr);
    glCompileShader(vertexShader);


    // Check if the shader compiled successfully
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    // If not, draw the log
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
            << infoLog << std::endl;
        return 0;
    }


    // Repeat the process for fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
            << infoLog << std::endl;
        return 0;
    }

    // Link the shaders together to form the final pipeline
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check linking errors as well
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n"
            << infoLog << std::endl;
        return 0;
    }

    return program;
}

int main() {
    ContextSettings ctxSettings;
    // Set OpenGL version (required for shader compatibility)
    ctxSettings.minorVersion = 3;
    ctxSettings.majorVersion = 3;
    sf::Window window(sf::VideoMode({ 800, 600 }), "OpenGL", sf::Style::Default, sf::State::Windowed, ctxSettings);

    window.setActive(true);

    if (glewInit() != GLEW_OK) {
        cout << "Failed to initialize GLEW!" << endl;
        return -1;
    }

    // Notice how a single array is used for both positon and color data
    GLuint triangleVAO = generateVertexArrayObject({ 
                                                    {-0.5f, -0.5f, 0.0f}, //pos[0]
                                                    {0.5f, -0.5f, 0.0f},  //pos[1]
                                                    {0.0f, 0.5f, 0.0f},   //pos[2]
                                                    {1.0f, 0.0f, 0.0f}, //color[0]
                                                    {0.0f, 1.0f, 0.0f}, //color[1]
                                                    {0.0f, 0.0f, 1.0f}  //color[2]
        }
    );

    if (GLenum err = glGetError() != 0) {
        cout << "Failed to send the triangle data to VRAM" << endl;
        return err;
    }

    // Set the current "model" or vertex array object
    glBindVertexArray(triangleVAO);

    GLuint shaderProgram = loadShaders(VERTEX_SHADER, FRAGMENT_SHADER);

    if (!shaderProgram) {
        return -1;
    }

    if (GLenum err = glGetError() != 0) {
        cout << "Failed to create shader program" << endl;
        return err;
    }

    // Set the current shading program (vertex + fragment shaders)
    glUseProgram(shaderProgram);

    bool running = true;
    while (running) {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                running = false;
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                glViewport(0, 0, resized->size.x, resized->size.y);
            }
        }

        // Reset the screen pixels !
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the triangle
        // Vertices offset sat to 0
        // Numebr of vertices sat to 3 !
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Update the window frame
        window.display();
    }
}