#include "Movement.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

Movement::Movement(Camera& cam, Map& m, float startX, float startY)
    : camera(cam), map(m), lastX(startX), lastY(startY), firstMouse(true) {
    // Altura inicial do piso
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

    // Alterna modo dev (noclip) com a tecla N
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
    
    // Obtém o setor atual e a altura do piso
    int curSecIdx = map.GetSectorAt(oldPos.x * 100.0f, oldPos.z * 100.0f);
    float curFloor = (curSecIdx != -1) ? (float)map.GetSectors()[curSecIdx].floorHeight * 0.01f : oldPos.y - PLAYER_EYE_HEIGHT;

    // Intenção de movimento
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

        // Verifica colisão e ajusta a posição com deslizamento (sliding)
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
                // Se esta porta estiver aberta, não bloqueia
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
                        
                        // Adiciona deslocamento dinâmico para portas
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

        // Altura do piso e Interpolação (Lerp)
        int newSecIdx = map.GetSectorAt(camera.Position.x * 100.0f, camera.Position.z * 100.0f);
        if (newSecIdx != -1) {
            targetFloorHeight = (float)map.GetSectors()[newSecIdx].floorHeight * 0.01f;
        }

        // Interpola a altura da câmera
        float currentY = camera.Position.y;
        float targetY = targetFloorHeight + PLAYER_EYE_HEIGHT;
        camera.Position.y = currentY + (targetY - currentY) * SMOOTH_FACTOR * deltaTime;
    } else {
        // Modo Dev (Noclip): voa para cima/baixo com Q/E
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.Position.y += currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.Position.y -= currentSpeed * deltaTime;
    }

    // --- INTERAÇÃO: ABRIR PORTAS ---
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceKeyWasPressed) {
            spaceKeyWasPressed = true;
            
            // Raycast frontal para encontrar uma interação de linha
            int hitLine = map.RayCastToLineDef(camera.Position, camera.Front);
            if (hitLine != -1) {
                const auto& line = map.GetLineDefs()[hitLine];
                
                // --- PORTAS (Teto) ---
                bool isDoor = (line.specialType == 1 || line.specialType == 9 || line.specialType == 11 || line.specialType == 14 ||
                               line.specialType == 26 || line.specialType == 27 || line.specialType == 28 ||
                               line.specialType == 31 || line.specialType == 32 || line.specialType == 33 || line.specialType == 34 ||
                               line.specialType == 46 || line.specialType == 63 || line.specialType == 103 ||
                               line.specialType == 114 || line.specialType == 115 || line.specialType == 117 || line.specialType == 118);
                
                // --- PISOS (Elevação) ---
                bool isFloor = (line.specialType == 18 || line.specialType == 19 || line.specialType == 20 || // Floor Raise
                                line.specialType == 36 || line.specialType == 37 || line.specialType == 38 || // W1/S1/WR Floor Raise
                                line.specialType == 22 || line.specialType == 23 ||
                                line.specialType == 101 || line.specialType == 102);

                if (isDoor || isFloor) {
                    std::vector<int> targetSectors;
                    
                    if (line.sectorTag == 0) {
                        // Local: verifica ambos os lados
                        int sR = (line.rightSideDef != -1) ? map.GetSideDefs()[line.rightSideDef].sector : -1;
                        int sL = (line.leftSideDef != -1) ? map.GetSideDefs()[line.leftSideDef].sector : -1;
                        
                        if (isDoor) {
                            if (sR != -1 && sL != -1) {
                                if (map.GetSectors()[sR].ceilingHeight < map.GetSectors()[sL].ceilingHeight) targetSectors.push_back(sR);
                                else targetSectors.push_back(sL);
                            } else if (sR != -1) targetSectors.push_back(sR);
                        } else {
                            if (sR != -1 && sL != -1) {
                                if (map.GetSectors()[sR].floorHeight < map.GetSectors()[sL].floorHeight) targetSectors.push_back(sR);
                                else targetSectors.push_back(sL);
                            } else if (sR != -1) targetSectors.push_back(sR);
                        }
                    } else {
                        targetSectors = map.GetSectorsByTag(line.sectorTag);
                    }

                    for (int animSecIdx : targetSectors) {
                        ActiveSectorAnim* existing = nullptr;
                        for (auto& a : activeAnims) {
                            if (a.sectorIndex == animSecIdx) {
                                existing = &a;
                                break;
                            }
                        }
                        
                        if (existing) {
                            // Se a porta estiver fechando ou aberta, reinicia o movimento de abertura (padrão DOOM)
                            if (existing->state == DoorState::CLOSING || (isDoor && existing->state == DoorState::OPEN)) {
                                existing->state = DoorState::OPENING;
                                if (isDoor) existing->targetY = map.GetHighestAdjacentCeiling(animSecIdx);
                                bool stay = (line.specialType == 31 || line.specialType == 32 || line.specialType == 118);
                                existing->waitTime = stay ? 999999.0f : 4.0f;
                                std::cout << "Reversing/Refreshing Sector Anim: " << animSecIdx << std::endl;
                            }
                            continue;
                        }

                        ActiveSectorAnim anim;
                        anim.sectorIndex = animSecIdx;
                        anim.state = DoorState::OPENING;
                        anim.animType = isDoor ? SectorAnimType::CEILING : SectorAnimType::FLOOR;
                        anim.speed = 70.0f;

                        if (isDoor) {
                            anim.startY = (float)map.GetSectors()[animSecIdx].ceilingHeight;
                            anim.targetY = map.GetHighestAdjacentCeiling(animSecIdx);
                            bool stay = (line.specialType == 31 || line.specialType == 32 || line.specialType == 118);
                            anim.waitTime = stay ? 999999.0f : 4.0f;
                        } else {
                            anim.startY = (float)map.GetSectors()[animSecIdx].floorHeight;
                            anim.targetY = map.GetNextHigherFloor(animSecIdx);
                            anim.waitTime = 999999.0f;
                        }

                        activeAnims.push_back(anim);
                        std::cout << "Activating Sector Anim: " << animSecIdx << " (Type: " << line.specialType << ", Prop: " << (isDoor ? "CEIL" : "FLOOR") << ", Target: " << anim.targetY << ")" << std::endl;
                    }
                } else if (line.specialType != 0) {
                    std::cout << "Interacted with Special Line: " << line.specialType << " (Tag: " << line.sectorTag << ") - Not a standard door/anim." << std::endl;
                }
            }
        }
    } else {
        spaceKeyWasPressed = false;
    }

    UpdateSectorAnims(deltaTime);
}

void Movement::UpdateSectorAnims(float deltaTime) {
    auto& ceilOffsets = map.GetCeilOffsets();
    auto& floorOffsets = map.GetFloorOffsets();
    const auto& sectors = map.GetSectors();

    for (auto it = activeAnims.begin(); it != activeAnims.end(); ) {
        bool isCeil = (it->animType == SectorAnimType::CEILING);
        // Atualiza a animação do setor
        float& currentOff = isCeil ? ceilOffsets[it->sectorIndex] : floorOffsets[it->sectorIndex];
        float baseHeight = isCeil ? (float)sectors[it->sectorIndex].ceilingHeight : (float)sectors[it->sectorIndex].floorHeight;
        float totalTargetOff = (it->targetY - baseHeight);

        if (it->state == DoorState::OPENING) {
            bool movingUp = (it->targetY > baseHeight + currentOff);
            float step = it->speed * deltaTime;
            if (!movingUp) step = -step;

            currentOff += step;
            if ((movingUp && currentOff >= totalTargetOff) || (!movingUp && currentOff <= totalTargetOff)) {
                currentOff = totalTargetOff;
                it->state = DoorState::OPEN;
            }
        } else if (it->state == DoorState::OPEN) {
            it->waitTime -= deltaTime;
            if (it->waitTime <= 0) {
                it->state = DoorState::CLOSING;
            }
        } else if (it->state == DoorState::CLOSING) {
            float step = it->speed * deltaTime;
            if (currentOff > 0.0f) {
                currentOff -= step;
                if (currentOff < 0.0f) currentOff = 0.0f;
            } else {
                currentOff += step;
                if (currentOff > 0.0f) currentOff = 0.0f;
            }

            if (currentOff == 0.0f) {
                it = activeAnims.erase(it);
                continue;
            }
        }
        ++it;
    }
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

    // Verifica se é uma porta
    if (line.specialType == 0) return;

    const auto& sides = map.GetSideDefs();
    const auto& sectors = map.GetSectors();
    
    int curSecIdx = map.GetSectorAt(camera.Position.x * 100.0f, camera.Position.z * 100.0f);
    
    // Determina os setores de destino (Local vs Remoto)
    std::vector<int> targetSectors;
    if (line.sectorTag == 0) {
        int sR = sides[line.rightSideDef].sector;
        int sL = (line.leftSideDef != -1) ? sides[line.leftSideDef].sector : -1;
        if (sR != -1 && sL != -1) {
            if (sectors[sR].ceilingHeight < sectors[sL].ceilingHeight) targetSectors.push_back(sR);
            else targetSectors.push_back(sL);
        } else if (sR != -1) targetSectors.push_back(sR);
        else if (sL != -1) targetSectors.push_back(sL);
    } else {
        targetSectors = map.GetSectorsByTag(line.sectorTag);
    }

    for (int doorSecIdx : targetSectors) {
        bool alreadyActive = false;
        for (auto& a : activeAnims) if (a.sectorIndex == doorSecIdx) alreadyActive = true;
        
        if (!alreadyActive) {
            ActiveSectorAnim anim;
            anim.sectorIndex = doorSecIdx;
            anim.state = DoorState::OPENING;
            anim.animType = SectorAnimType::CEILING;
            anim.startY = (float)map.GetSectors()[doorSecIdx].ceilingHeight;

            float curCeil = (curSecIdx != -1) ? (float)map.GetSectors()[curSecIdx].ceilingHeight : -1000.0f;
            anim.targetY = map.GetHighestAdjacentCeiling(doorSecIdx, curCeil);
            anim.waitTime = 4.0f;
            anim.speed = 70.0f;
            activeAnims.push_back(anim);
        }
    }
}
