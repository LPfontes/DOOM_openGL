#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "WADParser.h"
#include "Map.h"
#include "Scene.h"
#include "Camera.h"
#include "Movement.h"
#include "InputHandler.h"

// Dimensões da janela
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Câmera
Camera camera(glm::vec3(0.0f, 20.0f, 50.0f));

// Temporização (Timing)
float deltaTime = 0.0f;
float lastFrame = 0.0f;


// Função auxiliar para ler arquivo de shader
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
    // Inicializa o GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configura o GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Cria a Janela
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "DOOM OpenGL Simplificado", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // diz ao GLFW para capturar nosso mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializa o GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // --- Carrega os Shaders ---
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
                 std::cerr << "Erro de linkagem do shader: " << log << std::endl; } }
    glUseProgram(shaderProgram);

    // Associa a unidade de textura 0 (o padrão) ao uniform do shader
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    glActiveTexture(GL_TEXTURE0);

    {   // Escopo GL: todos os objetos GL são destruídos antes de glfwTerminate()
        // --- Carrega WAD e Mapa ---
        WADParser wad("assets/doom1.wad");
        if (!wad.Load()) { glfwTerminate(); return -1; }
        Map map("E1M1");
        if (!map.LoadFromWAD(wad)) { glfwTerminate(); return -1; }

        // Configuração da lógica de movimento
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

        // --- Posiciona a Câmera no ponto de início do Player 1 ---
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

        glm::mat4 view, projection;
        InputHandler inputHandler(camera, &map, &scene, &wad, &movement, view, projection);
        inputHandler.SetCallbacks(window);

        // Loop de Renderização
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
            view = camera.GetViewMatrix();
            GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
            glUniform3f(lightPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);

            projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            inputHandler.UpdateMatrices(view, projection);


            
            scene.Render(map.GetCeilOffsets(), map.GetFloorOffsets(), currentFrame, camera.Position, camera.Front, inputHandler.IsFlashlightOn());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }   // cena, mapa, wad são destruídos aqui — o contexto GL ainda está vivo

    glfwTerminate();
    return 0;
}
