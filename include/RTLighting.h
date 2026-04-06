#ifndef RT_LIGHTING_H
#define RT_LIGHTING_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "MapData.h"

struct GPULight {
    glm::vec4 posType;      // x, y, z, tipo (0: fixo, 1: oscilante, 2: pulso, 3: holofote)
    glm::vec4 colorInt;     // r, g, b, intensidade
    glm::vec4 dirCutoff;    // dx, dy, dz, corte (cutoff)
    float radius;
    float padding[3];
};

struct GPULine {
    glm::vec2 p1;
    glm::vec2 p2;
    float floor;
    float ceil;
    int sectorIdx;
    float padding;
};

class RTManager {
public:
    RTManager();
    ~RTManager();

    void Init();
    void SetFlashlight(const glm::vec3& pos, const glm::vec3& dir, bool on);
    void UpdateLights(const std::vector<WADThing>& things, const std::vector<WADSector>& sectors, int playerSector);
    void UpdateMapData(const std::vector<WADLineDef>& lines, const std::vector<WADVertex>& verts, const std::vector<WADSector>& sectors, const std::vector<WADSideDef>& sides);
    
    void Bind(GLuint shaderProgram);
    void Dispatch(int width, int height);

private:
    GLuint mLightSSBO = 0;
    GLuint mLineSSBO = 0;
    GLuint mOutputTex = 0;

    std::vector<GPULight> mLights;
    std::vector<GPULine> mLines;
};

#endif // RT_LIGHTING_H
