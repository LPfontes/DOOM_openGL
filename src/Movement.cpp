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

        for (int lineIdx = 0; lineIdx < (int)lineDefs.size(); ++lineIdx) {
            const auto& line = lineDefs[lineIdx];
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
                // If this door is open, don't block
                if (map.IsDoorOpen(lineIdx)) {
                    continue;
                }
                
                bool blocking = (line.leftSideDef == -1) || (line.flags & 0x0001);
                
                if (!blocking) {
                    int otherSideIdx = (line.rightSideDef != -1 && sides[line.rightSideDef].sector == curSecIdx) ? line.leftSideDef : line.rightSideDef;
                    if (otherSideIdx != -1) {
                        const auto& otherSec = sectors[sides[otherSideIdx].sector];
                        float otherFloor = (float)otherSec.floorHeight * 0.01f;
                        float otherCeil = (float)otherSec.ceilingHeight * 0.01f;
                        
                        // Add dynamic offset for doors
                        otherCeil += map.GetCeilOffsets()[sides[otherSideIdx].sector];
                        
                        if (otherFloor - curFloor > MAX_STEP_HEIGHT) blocking = true;
                        if (otherCeil - otherFloor < 0.50f) blocking = true;
                    }
                }

                if (blocking) {
                    // 1. Resolve a penetração (Empurra o jogador para fora da parede)
                    float penetration = collisionRadius - std::sqrt(distSq);
                    if (penetration > 0.0f) {
                        float wallLen = std::sqrt(lenSq);
                        // Calcula a normal da parede (-dz, dx)
                        float nx = -dz / wallLen;
                        float nz = dx / wallLen;
                        
                        // Garante que a normal aponte na direção do jogador
                        if ((px - closestX) * nx + (pz - closestZ) * nz < 0) {
                            nx = -nx;
                            nz = -nz;
                        }

                        // Empurra a posição futura
                        nextPos.x += (nx * penetration) * 0.01f;
                        nextPos.z += (nz * penetration) * 0.01f;
                    }

                    // 2. Calcula o deslizamento (Sliding) para não grudar na parede
                    float wallX = dx, wallZ = dz;
                    float wallLen = std::sqrt(lenSq);
                    float ux = wallX / wallLen, uz = wallZ / wallLen;

                    float dot = (moveVec.x * 100.0f * ux + moveVec.z * 100.0f * uz);
                    
                    // Atualiza o moveVec para que as próximas checagens no loop respeitem o slide
                    moveVec.x = (dot * ux) * 0.01f;
                    moveVec.z = (dot * uz) * 0.01f;
                    
                    px = nextPos.x * 100.0f;
                    pz = nextPos.z * 100.0f;
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

    // --- INTERACTION: OPEN DOORS ---
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceKeyWasPressed) {
            spaceKeyWasPressed = true;
            // Raycast forward to find a door (64 units)
            float rayLen = 64.0f; 
            float rx = camera.Position.x * 100.0f + camera.Front.x * rayLen;
            float rz = camera.Position.z * 100.0f + camera.Front.z * rayLen;
            float px = camera.Position.x * 100.0f;
            float pz = camera.Position.z * 100.0f;

            for (const auto& line : map.GetLineDefs()) {
                const auto& v1 = map.GetVertices()[line.v1];
                const auto& v2 = map.GetVertices()[line.v2];
                // Simple intersection or proximity check
                float dx = (float)v2.x - v1.x;
                float dz = (float)v2.y - v1.y;
                float lenSq = dx*dx + dz*dz;
                if (lenSq == 0) continue;
                float t = std::max(0.0f, std::min(1.0f, ((px - v1.x) * dx + (pz - v1.y) * dz) / lenSq));
                float cx = v1.x + t * dx, cz = v1.y + t * dz;
                float dSq = (px - cx)*(px - cx) + (pz - cz)*(pz - cz);

                if (dSq < rayLen * rayLen && (line.specialType == 1 || line.specialType == 31)) {
                    // It's a door!
                    int doorSecIdx = -1;
                    if (line.sectorTag == 0) {
                        // Local door: use back sector
                        doorSecIdx = (map.GetSideDefs()[line.rightSideDef].sector == curSecIdx) ? 
                                       map.GetSideDefs()[line.leftSideDef].sector : 
                                       map.GetSideDefs()[line.rightSideDef].sector;
                    } 
                    // ... remote door logic omitted for brevity ...

                    if (doorSecIdx != -1) {
                        bool alreadyActive = false;
                        for(auto& d : activeDoors) if(d.sectorIndex == doorSecIdx) alreadyActive = true;
                        
                        if (!alreadyActive) {
                            ActiveDoor door;
                            door.sectorIndex = doorSecIdx;
                            door.state = DoorState::OPENING;
                            // Doom doors open to lowest adjacent ceiling - 4
                            door.targetY = ((float)map.GetSectors()[doorSecIdx].ceilingHeight + 100.0f) * 0.01f; // Simplified target
                            door.waitTime = 4.0f;
                            activeDoors.push_back(door);
                            std::cout << "Opening Door Sector: " << doorSecIdx << std::endl;
                        }
                    }
                }
            }
        }
    } else {
        spaceKeyWasPressed = false;
    }

    // --- ANIMATE DOORS ---
    auto& ceilOffsets = map.GetCeilOffsets();
    for (auto it = activeDoors.begin(); it != activeDoors.end(); ) {
        float& off = ceilOffsets[it->sectorIndex];
        float speed = 2.0f * deltaTime; // Units per second (scaled)

        if (it->state == DoorState::OPENING) {
            off += speed;
            if (off >= (it->targetY - (float)map.GetSectors()[it->sectorIndex].ceilingHeight * 0.01f)) {
                off = it->targetY - (float)map.GetSectors()[it->sectorIndex].ceilingHeight * 0.01f;
                it->state = DoorState::OPEN;
            }
        } else if (it->state == DoorState::OPEN) {
            it->waitTime -= deltaTime;
            if (it->waitTime <= 0) it->state = DoorState::CLOSING;
        } else if (it->state == DoorState::CLOSING) {
            off -= speed;
            if (off <= 0) {
                off = 0;
                it = activeDoors.erase(it);
                continue;
            }
        }
        ++it;
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

void Movement::OpenDoorByLineDefIndex(int lineDefIdx) {
    if (lineDefIdx < 0 || lineDefIdx >= (int)map.GetLineDefs().size())
        return;
    
    const auto& line = map.GetLineDefs()[lineDefIdx];

    // portas especiais // abre porta por missao
    if (line.specialType != 1 && line.specialType != 31 && line.specialType != 117 && line.specialType != 118) {
        return;
    }

    const auto& sides = map.GetSideDefs();
    const auto& sectors = map.GetSectors();
    
    // Find which side of the door the player is on
    int curSecIdx = map.GetSectorAt(camera.Position.x * 100.0f, camera.Position.z * 100.0f);
    if (curSecIdx == -1) return;
    
    // Determine which sector is the door sector
    int doorSecIdx = -1;
    if (line.rightSideDef != -1 && line.leftSideDef != -1) {
        int rightSec = sides[line.rightSideDef].sector;
        int leftSec = sides[line.leftSideDef].sector;
        
        // The door sector is the one we're NOT in
        doorSecIdx = (rightSec == curSecIdx) ? leftSec : rightSec;
    }
    
    if (doorSecIdx == -1) return;
    
    // Check if door is already animating or open
    bool alreadyActive = false;
    for (auto& d : activeDoors) {
        if (d.sectorIndex == doorSecIdx) {
            alreadyActive = true;
            break;
        }
    }
    
    if (!alreadyActive) {
        ActiveDoor door;
        door.sectorIndex = doorSecIdx;
        door.state = DoorState::OPENING;
        // Calculate target ceiling (open all the way)
        door.targetY = ((float)sectors[doorSecIdx].ceilingHeight + 100.0f) * 0.01f;
        door.waitTime = 2.0f;  // Shorter wait time for mouse-opened doors
        activeDoors.push_back(door);
    }
}
