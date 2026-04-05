#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Map;
class Scene;
class WADParser;
class Camera;
class Movement;

class InputHandler {
public:
    InputHandler(Camera& cam, Map* map, Scene* scene, WADParser* wad, Movement* movement, glm::mat4& view, glm::mat4& projection);

    void SetMouseCallback(GLFWwindow* window);
    void UpdateMatrices(const glm::mat4& view, const glm::mat4& projection);

private:
    Camera& camera;
    Map* gMap;
    Scene* gScene;
    WADParser* gWad;
    Movement* gMovement;
    glm::mat4* gView;
    glm::mat4* gProjection;

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static InputHandler* instance;
};

#endif // INPUT_HANDLER_H