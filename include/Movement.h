#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "Camera.h"
#include "Map.h"
#include <GLFW/glfw3.h>

class Movement {
public:
    Movement(Camera& camera, Map& map, float startX, float startY);

    void ProcessInput(GLFWwindow* window, float deltaTime);
    void ProcessMouse(double xpos, double ypos);
    void ProcessScroll(double yoffset);
    
    // Open a door by linedef index with animation
    void OpenDoorByLineDefIndex(int lineDefIdx);

private:
    Camera& camera;
    Map& map;
    float lastX;
    float lastY;
    bool firstMouse;

    // Movement constants
    const float BASE_SPEED = 4.0f;
    const float RUN_MULTIPLIER = 2.0f;
    const float PLAYER_RADIUS = 0.16f; // Original Doom radius (16 units scaled by 0.01)
    const float COLLISION_BUFFER = 0.005f;

    const float PLAYER_EYE_HEIGHT = 0.5f;
    const float MAX_STEP_HEIGHT = 0.50f;
    const float SMOOTH_FACTOR = 10.0f;

    float targetFloorHeight = 0.0f;
    bool devMode = false;
    bool nKeyWasPressed = false;

    enum class SectorAnimType { CEILING, FLOOR };
    enum class DoorState { CLOSED, OPENING, OPEN, CLOSING };
    
    struct ActiveSectorAnim {
        int sectorIndex;
        SectorAnimType animType;
        DoorState state;
        float startY;
        float targetY;
        float waitTime;
        float speed;
    };
    
    std::vector<ActiveSectorAnim> activeAnims;
    bool spaceKeyWasPressed = false;

    void UpdateSectorAnims(float deltaTime);

    bool CheckCollision(glm::vec3 nextPos);
};

#endif // MOVEMENT_H
