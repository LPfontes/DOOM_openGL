#include "RTLighting.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "WADParser.h"

RTManager::RTManager() {}

RTManager::~RTManager() {
    if (mLightSSBO) glDeleteBuffers(1, &mLightSSBO);
    if (mLineSSBO) glDeleteBuffers(1, &mLineSSBO);
    if (mOutputTex) glDeleteTextures(1, &mOutputTex);
}

void RTManager::Init() {
    glGenBuffers(1, &mLightSSBO);
    glGenBuffers(1, &mLineSSBO);
}

void RTManager::SetFlashlight(const glm::vec3& pos, const glm::vec3& dir, bool on) {
    GPULight fl;
    // Escala a posição da câmera (que já está em 0.01) de volta para unidades DOOM para o raytracer
    fl.posType = glm::vec4(pos.x * 100.0f, pos.y * 100.0f, pos.z * 100.0f, 3.0f); // tipo 3: holofote
    fl.colorInt = glm::vec4(1.0f, 0.95f, 0.8f, on ? 3.5f : 0.0f); // Usa intensidade zero se estiver DESLIGADO
    fl.dirCutoff = glm::vec4(dir.x, dir.y, dir.z, 0.92f); // cone de aprox. 22.5 graus
    fl.radius = 800.0f;
    
    if (mLights.empty()) {
        mLights.push_back(fl);
    } else if (mLights[0].posType.w == 3.0f) {
        mLights[0] = fl;
    } else {
        mLights.insert(mLights.begin(), fl);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mLights.size() * sizeof(GPULight), mLights.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void RTManager::UpdateLights(const std::vector<WADThing>& things, const std::vector<WADSector>& sectors, int playerSector) {
    bool hasFlashlight = !mLights.empty() && mLights[0].posType.w == 3.0f;
    GPULight fl;
    if (hasFlashlight) fl = mLights[0];

    mLights.clear();
    if (hasFlashlight) mLights.push_back(fl);

    for (const auto& t : things) {
        GPULight gl;
        bool isLight = false;
        gl.posType = glm::vec4(t.x, 32.0f, t.y, 0.0f); 
        gl.radius = 150.0f;
        gl.dirCutoff = glm::vec4(0,0,0,0);

        switch (t.type) {
            case 34: // Vela (Candle)
                gl.colorInt = glm::vec4(1.0f, 0.9f, 0.6f, 1.5f); 
                gl.radius = 100.0f; 
                isLight = true;
                break;
            case 35: // Candelabro (Candelabra)
                gl.colorInt = glm::vec4(1.0f, 0.85f, 0.5f, 3.5f); 
                gl.radius = 250.0f; 
                isLight = true;
                break;
            case 2024: // Tocha Verde (Green Torch)
                gl.colorInt = glm::vec4(0.4f, 1.0f, 0.4f, 4.5f); 
                gl.posType.w = 1.0f; 
                gl.radius = 350.0f;   
                isLight = true;
                break;
            case 2025: // Tocha Vermelha (Red Torch)
                gl.colorInt = glm::vec4(1.0f, 0.3f, 0.3f, 4.5f);
                gl.posType.w = 1.0f;
                gl.radius = 350.0f;
                isLight = true;
                break;
            case 2026: // Tocha Azul (Blue Torch)
                gl.colorInt = glm::vec4(0.3f, 0.5f, 1.0f, 4.5f);
                gl.posType.w = 1.0f;
                gl.radius = 350.0f;
                isLight = true;
                break;
            case 55: case 56: case 57: 
                gl.colorInt = (t.type == 55) ? glm::vec4(0.3f, 0.5f, 1.0f, 3.0f) :
                              (t.type == 56) ? glm::vec4(1.0f, 0.3f, 0.3f, 3.0f) :
                                               glm::vec4(0.4f, 1.0f, 0.4f, 3.0f);
                gl.posType.w = 1.0f;
                gl.radius = 200.0f; 
                isLight = true;
                break;
            case 48: // Pilar tecnológico (Tech pillar)
                gl.colorInt = glm::vec4(0.5f, 0.7f, 1.0f, 2.0f);
                gl.posType.w = 2.0f;
                gl.radius = 200.0f;
                isLight = true;
                break;
        }
        if (isLight) mLights.push_back(gl);
    }
    if (!mLights.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mLights.size() * sizeof(GPULight), mLights.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void RTManager::UpdateMapData(const std::vector<WADLineDef>& lines, const std::vector<WADVertex>& verts, const std::vector<WADSector>& sectors, const std::vector<WADSideDef>& sides) {
    mLines.clear();
    for (const auto& l : lines) {
        GPULine glue;
        glue.p1 = glm::vec2(verts[l.v1].x, verts[l.v1].y);
        glue.p2 = glm::vec2(verts[l.v2].x, verts[l.v2].y);
        
        if (l.rightSideDef != -1) {
            int sR = sides[l.rightSideDef].sector;
            int sL = (l.leftSideDef != -1) ? sides[l.leftSideDef].sector : -1;
            
            // Escolhe o setor com o teto mais baixo (geralmente a porta)
            int sIdx = sR;
            if (sL != -1 && sectors[sL].ceilingHeight < sectors[sR].ceilingHeight) {
                sIdx = sL;
            }

            glue.floor = (float)sectors[sIdx].floorHeight;
            glue.ceil = (float)sectors[sIdx].ceilingHeight;
            glue.sectorIdx = sIdx;
        } else {
            glue.floor = -1000.0f;
            glue.ceil = 1000.0f;
            glue.sectorIdx = -1;
        }
        mLines.push_back(glue);
    }

    if (!mLines.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLineSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mLines.size() * sizeof(GPULine), mLines.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void RTManager::Bind(GLuint shaderProgram) {
    // Dados de luz para binding 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mLightSSBO);
    // Dados de Linha/Geometria para binding 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mLineSSBO);
    
    glUniform1i(glGetUniformLocation(shaderProgram, "uNumLights"), (int)mLights.size());
    glUniform1i(glGetUniformLocation(shaderProgram, "uNumLines"), (int)mLines.size());
}
