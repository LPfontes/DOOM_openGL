#include "Movement.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

Movement::Movement(Camera& cam, Map& m, float startX, float startY)
    : camera(cam), map(m), lastX(startX), lastY(startY), firstMouse(true) {
    // Initial floor height
    int secIdx = map.GetSectorAt(camera.Position.x * 100.0f, camera.Position.z * 100.0f);
    if (secIdx != -1) {
        targetFloorHeight = (float)map.GetSectors()[secIdx].floorHeight * 0.01f;
        camera.Position.y = targetFloorHeight + PLAYER_EYE_HEIGHT;
    }
}

void Movement::ProcessInput(GLFWwindow* window, float deltaTime) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentSpeed = BASE_SPEED;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        currentSpeed *= RUN_MULTIPLIER;

    // Toggle devMode with N key
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (!nKeyWasPressed) {
            devMode = !devMode;
            std::cout << "Dev Mode: " << (devMode ? "ON (Noclip)" : "OFF") << std::endl;
            nKeyWasPressed = true;
        }
    } else {
        nKeyWasPressed = false;
    }

    if (devMode) currentSpeed *= 3.0f; 
    camera.MovementSpeed = currentSpeed;

    glm::vec3 oldPos = camera.Position;
    
    // Get current sector and floor height
    int curSecIdx = map.GetSectorAt(oldPos.x * 100.0f, oldPos.z * 100.0f);
    float curFloor = (curSecIdx != -1) ? (float)map.GetSectors()[curSecIdx].floorHeight * 0.01f : oldPos.y - PLAYER_EYE_HEIGHT;

    // Movement intent
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, true);

    if (!devMode) {
        glm::vec3 nextPos = camera.Position;
        glm::vec3 moveVec = nextPos - oldPos;

        // Check collision and adjust pos with sliding
        const auto& vertices = map.GetVertices();
        const auto& lineDefs = map.GetLineDefs();
        const auto& sectors  = map.GetSectors();
        const auto& sides    = map.GetSideDefs();

        float px = nextPos.x * 100.0f;
        float pz = nextPos.z * 100.0f;
        float pr = PLAYER_RADIUS * 100.0f;

        for (const auto& line : lineDefs) {
            const auto& v1 = vertices[line.v1];
            const auto& v2 = vertices[line.v2];

            float ax = (float)v1.x, az = (float)v1.y;
            float bx = (float)v2.x, bz = (float)v2.y;

            float dx = bx - ax, dz = bz - az;
            float lenSq = dx*dx + dz*dz;
            if (lenSq == 0) continue;

            float t = std::max(0.0f, std::min(1.0f, ((px - ax) * dx + (pz - az) * dz) / lenSq));
            float closestX = ax + t * dx, closestZ = az + t * dz;
            float distSq = (px - closestX)*(px - closestX) + (pz - closestZ)*(pz - closestZ);
            float collisionRadius = pr + (COLLISION_BUFFER * 100.0f);

            if (distSq < collisionRadius * collisionRadius) {
                bool blocking = (line.leftSideDef == -1) || (line.flags & 0x0001);
                
                if (!blocking) {
                    int otherSideIdx = (line.rightSideDef != -1 && sides[line.rightSideDef].sector == curSecIdx) ? line.leftSideDef : line.rightSideDef;
                    if (otherSideIdx != -1) {
                        const auto& otherSec = sectors[sides[otherSideIdx].sector];
                        float otherFloor = (float)otherSec.floorHeight * 0.01f;
                        float otherCeil = (float)otherSec.ceilingHeight * 0.01f;
                        
                        if (otherFloor - curFloor > MAX_STEP_HEIGHT) blocking = true;
                        if (otherCeil - otherFloor < 0.50f) blocking = true;
                    }
                }

                if (blocking) {
                    float wallX = dx, wallZ = dz;
                    float wallLen = std::sqrt(lenSq);
                    float ux = wallX / wallLen, uz = wallZ / wallLen;

                    float dot = (moveVec.x * 100.0f * ux + moveVec.z * 100.0f * uz);
                    nextPos.x = oldPos.x + (dot * ux) * 0.01f;
                    nextPos.z = oldPos.z + (dot * uz) * 0.01f;
                    
                    px = nextPos.x * 100.0f;
                    pz = nextPos.z * 100.0f;
                    moveVec = nextPos - oldPos;
                }
            }
        }

        camera.Position = nextPos;

        // Floor height and Lerp
        int newSecIdx = map.GetSectorAt(camera.Position.x * 100.0f, camera.Position.z * 100.0f);
        if (newSecIdx != -1) {
            targetFloorHeight = (float)map.GetSectors()[newSecIdx].floorHeight * 0.01f;
        }

        // Interpolate camera height
        float currentY = camera.Position.y;
        float targetY = targetFloorHeight + PLAYER_EYE_HEIGHT;
        camera.Position.y = currentY + (targetY - currentY) * SMOOTH_FACTOR * deltaTime;
    } else {
        // Dev Mode (Noclip): Fly up/down with Q/E
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.Position.y += currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.Position.y -= currentSpeed * deltaTime;
    }
}

bool Movement::CheckCollision(glm::vec3 nextPos) {
    // This is now handled inline in ProcessInput for sliding, 
    // but we can keep it as a simple check if needed.
    return false; 
}

void Movement::ProcessMouse(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Movement::ProcessScroll(double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
