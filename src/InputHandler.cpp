#include <glad/glad.h>
#include "InputHandler.h"
#include "Map.h"
#include "Scene.h"
#include "WADParser.h"
#include "Camera.h"
#include "Movement.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

InputHandler* InputHandler::instance = nullptr;

InputHandler::InputHandler(Camera& cam, Map* map, Scene* scene, WADParser* wad, Movement* movement, glm::mat4& view, glm::mat4& projection)
    : camera(cam), gMap(map), gScene(scene), gWad(wad), gMovement(movement), gView(&view), gProjection(&projection) {
    instance = this;
}

void InputHandler::SetCallbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetKeyCallback(window, KeyCallback);
}

void InputHandler::UpdateMatrices(const glm::mat4& view, const glm::mat4& projection) {
    *gView = view;
    *gProjection = projection;
}

void InputHandler::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!instance || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    if (!instance->gMap || !instance->gMovement || !instance->gWad)
        return;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // Sempre usa o centro da tela (como o crosshair do Minecraft)
    double xpos = width / 2.0;
    double ypos = height / 2.0;

    glm::vec4 viewport(0.0f, 0.0f, (float)width, (float)height);
    glm::vec3 winPos((float)xpos, (float)(height - ypos), 0.0f);
    glm::vec3 nearPos = glm::unProject(winPos, *instance->gView, *instance->gProjection, viewport);
    glm::vec3 rayDir3D = glm::normalize(nearPos - instance->camera.Position);
    glm::vec2 rayDir2D = glm::normalize(glm::vec2(rayDir3D.x, rayDir3D.y));
    glm::vec3 rayDir(rayDir2D.x, rayDir2D.y, 0.0f);

    int hitLine = instance->gMap->RayCastToLineDef(instance->camera.Position, rayDir);
    if (hitLine != -1 && instance->gMovement) {
        instance->gMovement->OpenDoorByLineDefIndex(hitLine);
    }
}
void InputHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!instance || action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_F) {
        instance->mFlashlightOn = !instance->mFlashlightOn;
        std::cout << "Flashlight: " << (instance->mFlashlightOn ? "ON" : "OFF") << std::endl;
    }
}
