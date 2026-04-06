#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <vector>
#include <map>
#include <string>
#include "Map.h"
#include "WADParser.h"
#include "RTLighting.h"

struct Vertex3D {
    float x, y, z;
    float u, v;
    float r, g, b;  // modulação de luz (cor)
    float sectorIndex; // ID do Setor (0..N)
    float vertexType;  // 0: Piso, 1: Teto, 2: Topo da Parede, 3: Base da Parede
    float invTexHeight; // 1.0 / Altura da Textura (para rolagem UV)
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
    void Render(const std::vector<float>& ceilOffsets, const std::vector<float>& floorOffsets, float time, const glm::vec3& camPos, const glm::vec3& camDir, bool flashlightOn);

private:
    std::vector<DrawBatch> mBatches;
    std::map<std::string, TexEntry> mTexCache;
    GLuint mFallbackTex = 0;

    RTManager mRTManager;

    GLuint CreateGLTexture(const std::vector<uint8_t>& rgb, int w, int h);
    TexEntry GetOrLoadWallTex(const char* name8, WADParser& wad);
    TexEntry GetOrLoadFlatTex(const char* name8, WADParser& wad);
};

#endif // SCENE_H
