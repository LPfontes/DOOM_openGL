#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <vector>
#include "Map.h"

struct Vertex3D {
    float x, y, z;
    float r, g, b;
};

class Scene {
public:
    Scene();
    ~Scene();

    void GenerateFromMap(const Map& map);
    void Render();

private:
    GLuint mVAO, mVBO;
    int mVertexCount;
};

#endif // SCENE_H
