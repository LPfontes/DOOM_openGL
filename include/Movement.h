#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "Camera.h"
#include <GLFW/glfw3.h>

class Movement {
public:
    Movement(Camera& camera, float startX, float startY);

    void ProcessInput(GLFWwindow* window, float deltaTime);
    void ProcessMouse(double xpos, double ypos);
    void ProcessScroll(double yoffset);

private:
    Camera& camera;
    float lastX;
    float lastY;
    bool firstMouse;
};

#endif // MOVEMENT_H
