#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <vector>
#include <map>
#include <string>
#include "Map.h"
#include "WADParser.h"

struct Vertex3D {
    float x, y, z;
    float u, v;
    float r, g, b;  // light modulation (lightLevel/255 per channel)
};

struct DrawBatch {
    GLuint vao, vbo;
    GLuint texId;
    int count;
};

struct TexEntry {
    GLuint id;
    int w, h;
};

class Scene {
public:
    Scene();
    ~Scene();

    void GenerateFromMap(const Map& map, WADParser& wad);
    void Render();

private:
    std::vector<DrawBatch> mBatches;
    std::map<std::string, TexEntry> mTexCache;
    GLuint mFallbackTex = 0;

    GLuint CreateGLTexture(const std::vector<uint8_t>& rgb, int w, int h);
    TexEntry GetOrLoadWallTex(const char* name8, WADParser& wad);
    TexEntry GetOrLoadFlatTex(const char* name8, WADParser& wad);
};

#endif // SCENE_H
