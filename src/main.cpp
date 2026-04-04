#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "WADParser.h"
#include "Map.h"
#include "Scene.h"
#include "Camera.h"

// Camera
Camera camera(glm::vec3(0.0f, 20.0f, 50.0f));

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

#include "Movement.h"

// Helper function to read shader file
std::string readShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(const char* source, GLuint type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(shader, 512, NULL, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}



int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simplified DOOM OpenGL", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // --- Load Shaders ---
    std::string vCode = readShaderFile("assets/shaders/vertex.glsl");
    std::string fCode = readShaderFile("assets/shaders/fragment.glsl");
    GLuint vShader = compileShader(vCode.c_str(), GL_VERTEX_SHADER);
    GLuint fShader = compileShader(fCode.c_str(), GL_FRAGMENT_SHADER);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);
    { GLint ok; glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
      if (!ok) { char log[512]; glGetProgramInfoLog(shaderProgram, 512, NULL, log);
                 std::cerr << "Shader link error: " << log << std::endl; } }
    glUseProgram(shaderProgram);

    // Bind texture unit 0 (the default) to the shader uniform
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    glActiveTexture(GL_TEXTURE0);

    {   // GL scope: all GL objects destroyed before glfwTerminate()
        // --- Load WAD and Map ---
        WADParser wad("assets/doom1.wad");
        if (!wad.Load()) { glfwTerminate(); return -1; }
        Map map("E1M1");
        if (!map.LoadFromWAD(wad)) { glfwTerminate(); return -1; }

        // Movement logic setup
        Movement movement(camera, map, SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
        glfwSetWindowUserPointer(window, &movement);

        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
            Movement* m = static_cast<Movement*>(glfwGetWindowUserPointer(window));
            if (m) m->ProcessMouse(xpos, ypos);
        });
        glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
            Movement* m = static_cast<Movement*>(glfwGetWindowUserPointer(window));
            if (m) m->ProcessScroll(yoffset);
        });

        // --- Position Camera at Player 1 Start ---
        const auto& things = map.GetThings();
        for (const auto& t : things) {
            if (t.type == 1) {
                camera.Position = glm::vec3((float)t.x * 0.01f, 40.0f * 0.01f, (float)t.y * 0.01f);
                camera.Yaw = (float)t.angle;
                break;
            }
        }

        Scene scene;
        scene.GenerateFromMap(map, wad);

        // Render Loop
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            movement.ProcessInput(window, deltaTime);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(shaderProgram);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.01f));
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            scene.Render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }   // scene, map, wad destroyed here — GL context still alive

    glfwTerminate();
    return 0;
}
