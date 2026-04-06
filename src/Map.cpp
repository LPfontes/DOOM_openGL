#include "Map.h"
#include <iostream>
#include <cstring>
#include <limits>
#include <glm/glm.hpp>

Map::Map(const std::string& name) : mName(name) {}

bool Map::LoadFromWAD(WADParser& parser) {
    int mapIndex = parser.FindLump(mName);
    if (mapIndex == -1) {
        std::cerr << "Map " << mName << " not found in WAD." << std::endl;
        return false;
    }

    // Mapas de Doom têm uma ordem específica de lumps seguindo o nome do mapa
    // 1: THINGS, 2: LINEDEFS, 3: SIDEDEFS, 4: VERTEXES, 5: SEGS, 6: SSECTORS, 7: NODES, 8: SECTORS, 9: REJECT, 10: BLOCKMAP
    
    // Carrega Coisas/Things (Índice + 1)
    auto thingData = parser.ReadLumpData(mapIndex + 1);
    int numThings = thingData.size() / sizeof(WADThing);
    mThings.resize(numThings);
    memcpy(mThings.data(), thingData.data(), thingData.size());

    // Carrega Vértices (Índice + 4)
    auto vertexData = parser.ReadLumpData(mapIndex + 4);

    int numVertices = vertexData.size() / sizeof(WADVertex);
    mVertices.resize(numVertices);
    memcpy(mVertices.data(), vertexData.data(), vertexData.size());

    // Carrega LineDefs (Índice + 2)
    auto lineData = parser.ReadLumpData(mapIndex + 2);
    int numLines = lineData.size() / sizeof(WADLineDef);
    mLineDefs.resize(numLines);
    memcpy(mLineDefs.data(), lineData.data(), lineData.size());

    // Carrega SideDefs (Índice + 3)
    auto sideData = parser.ReadLumpData(mapIndex + 3);
    int numSides = sideData.size() / sizeof(WADSideDef);
    mSideDefs.resize(numSides);
    memcpy(mSideDefs.data(), sideData.data(), sideData.size());

    // Carrega Setores/Sectors (Índice + 8)
    auto sectorData = parser.ReadLumpData(mapIndex + 8);
    int numSectors = sectorData.size() / sizeof(WADSector);
    mSectors.resize(numSectors);
    memcpy(mSectors.data(), sectorData.data(), sectorData.size());

    // Carrega Segs (Índice + 5)
    auto segData = parser.ReadLumpData(mapIndex + 5);
    int numSegs = segData.size() / sizeof(WADSeg);
    mSegs.resize(numSegs);
    memcpy(mSegs.data(), segData.data(), segData.size());

    // Carrega Sub-setores/SubSectors (Índice + 6)
    auto ssectorData = parser.ReadLumpData(mapIndex + 6);
    int numSSectors = ssectorData.size() / sizeof(WADSubSector);
    mSubSectors.resize(numSSectors);
    memcpy(mSubSectors.data(), ssectorData.data(), ssectorData.size());

    // Carrega Nós/Nodes (Índice + 7)
    auto nodeData = parser.ReadLumpData(mapIndex + 7);
    int numNodes = nodeData.size() / sizeof(WADNode);
    mNodes.resize(numNodes);
    memcpy(mNodes.data(), nodeData.data(), nodeData.size());

    std::cout << "Loaded map: " << mName << std::endl;

    std::cout << "Vertices: " << mVertices.size() << std::endl;
    std::cout << "LineDefs: " << mLineDefs.size() << std::endl;
    std::cout << "Sectors:  " << mSectors.size() << std::endl;

    mCeilOffsets.assign(mSectors.size(), 0.0f);
    mFloorOffsets.assign(mSectors.size(), 0.0f);

    return true;
}

int Map::GetSectorAt(float x, float y) const {
    if (mNodes.empty()) return -1;

    int cur = (int)mNodes.size() - 1;
    while (!(cur & 0x8000)) {
        const auto& node = mNodes[cur];
        // Matemática da linha de partição (dx, dy são os componentes do vetor)
        // Calculamos em qual lado o ponto está em relação à linha de partição
        double dx = (double)x - node.x;
        double dy = (double)y - node.y;
        double cross = dx * (double)node.dy - dy * (double)node.dx;
        
        // Lógica do Doom: se (node.dy * dx - dy * node.dx > 0) então filho 0 (frente)
        if (cross > 0) cur = node.children[0];
        else cur = node.children[1];
    }

    // Sub-setor encontrado
    int ssecIdx = cur & 0x7FFF;
    if (ssecIdx >= mSubSectors.size()) return -1;
    const auto& ssec = mSubSectors[ssecIdx];
    
    // Cada seg em um sub-setor pertence ao mesmo setor
    if (ssec.numSegs > 0) {
        const auto& seg = mSegs[ssec.firstSeg];
        if (seg.lineDef != -1) {
            const auto& line = mLineDefs[seg.lineDef];
            int sideIdx = (seg.side == 0) ? line.rightSideDef : line.leftSideDef;
            if (sideIdx != -1) {
                return mSideDefs[sideIdx].sector;
            }
        }
    }

    return -1;
}

static bool IsDoorLine(const WADLineDef& line) {
    switch (line.specialType) {
        case 1: case 2: case 7: case 8: case 9: case 10: case 11:
            return true;
        default:
            return false;
    }
}

void Map::ToggleDoor(int lineDefIdx) {
    if (lineDefIdx < 0 || lineDefIdx >= (int)mLineDefs.size())
        return;

    const auto& line = mLineDefs[lineDefIdx];
    if (!IsDoorLine(line))
        return;

    mDoorStates[lineDefIdx] = !IsDoorOpen(lineDefIdx);
}

bool Map::IsDoorOpen(int lineDefIdx) const {
    auto it = mDoorStates.find(lineDefIdx);
    return it != mDoorStates.end() && it->second;
}


static bool RayIntersectsSegment2D(const glm::vec2& rayOrigin, const glm::vec2& rayDir,
                                    const glm::vec2& p1, const glm::vec2& p2,
                                    float& outDistance) {
    // Intersecção de Raio (Ray) com Segmento 2D
    glm::vec2 seg = p2 - p1;
    float denom = rayDir.x * seg.y - rayDir.y * seg.x;
    if (std::fabs(denom) < 1e-6f)
        return false;

    glm::vec2 diff = p1 - rayOrigin;
    float t = (diff.x * seg.y - diff.y * seg.x) / denom;
    if (t < 0.0f)
        return false;

    float u = (diff.x * rayDir.y - diff.y * rayDir.x) / denom;
    if (u < 0.0f || u > 1.0f)
        return false;

    outDistance = t;
    return true;
}

int Map::RayCastToLineDef(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const {
    float minDist = std::numeric_limits<float>::max();
    int hitLine = -1;
    float maxDistance = 5.5f; 

    glm::vec2 origin2D(rayOrigin.x, rayOrigin.z);
    
    float angles[] = { 0.0f, -0.174f, 0.174f }; // Ângulos de amostragem

    for (float angle : angles) {
        float cosA = cosf(angle);
        float sinA = sinf(angle);
        glm::vec2 baseDir(rayDir.x, rayDir.z);
        if (glm::length(baseDir) < 1e-6f) continue;
        baseDir = glm::normalize(baseDir);

        glm::vec2 dir2D(
            baseDir.x * cosA - baseDir.y * sinA,
            baseDir.x * sinA + baseDir.y * cosA
        );

        for (int i = 0; i < (int)mLineDefs.size(); ++i) {
            const auto& line = mLineDefs[i];
            const auto& v1 = mVertices[line.v1];
            const auto& v2 = mVertices[line.v2];
            glm::vec2 p1(v1.x * 0.01f, v1.y * 0.01f);
            glm::vec2 p2(v2.x * 0.01f, v2.y * 0.01f);

            float dist;
            if (RayIntersectsSegment2D(origin2D, dir2D, p1, p2, dist) && dist < minDist && dist <= maxDistance) {
                minDist = dist;
                hitLine = i;
            }
        }
    }
    return hitLine;
}

float Map::GetHighestAdjacentCeiling(int sectorIdx, float triggeringCeil) const {
    if (sectorIdx < 0 || sectorIdx >= (int)mSectors.size()) return 0.0f;
    
    float maxCeil = -32768.0f;
    bool foundAdjacent = false;

    for (const auto& line : mLineDefs) {
        if (line.rightSideDef == -1 || line.leftSideDef == -1) continue;
        
        int secR = mSideDefs[line.rightSideDef].sector;
        int secL = (line.leftSideDef != -1) ? mSideDefs[line.leftSideDef].sector : -1;

        if (secR == sectorIdx || secL == sectorIdx) {
            int otherSecIdx = (secR == sectorIdx) ? secL : secR;
            if (otherSecIdx == -1) continue;
            
            float otherCeil = (float)mSectors[otherSecIdx].ceilingHeight;
            if (otherCeil > maxCeil) {
                maxCeil = otherCeil;
                foundAdjacent = true;
            }
        }
    }
    
    // Doom: sobe até 4 unidades abaixo do teto vizinho mais alto
    return foundAdjacent ? (maxCeil - 4.0f) : (float)mSectors[sectorIdx].ceilingHeight;
}

float Map::GetNextHigherFloor(int sectorIdx) const {
    float curFloor = (float)mSectors[sectorIdx].floorHeight;
    float nextFloor = curFloor;
    bool found = false;

    for (const auto& line : mLineDefs) {
        if (line.rightSideDef == -1 || line.leftSideDef == -1) continue;
        int sR = mSideDefs[line.rightSideDef].sector;
        int sL = mSideDefs[line.leftSideDef].sector;

        int neighborIdx = -1;
        if (sR == sectorIdx) neighborIdx = sL;
        else if (sL == sectorIdx) neighborIdx = sR;

        if (neighborIdx != -1) {
            float nFloor = (float)mSectors[neighborIdx].floorHeight;
            if (nFloor > curFloor) {
                if (!found || nFloor < nextFloor) {
                    nextFloor = nFloor;
                    found = true;
                }
            }
        }
    }
    return nextFloor;
}

float Map::GetLowestAdjacentFloor(int sectorIdx) const {
    float lowest = (float)mSectors[sectorIdx].floorHeight;
    bool first = true;

    for (const auto& line : mLineDefs) {
        if (line.rightSideDef == -1 || line.leftSideDef == -1) continue;
        int sR = mSideDefs[line.rightSideDef].sector;
        int sL = mSideDefs[line.leftSideDef].sector;

        int neighborIdx = -1;
        if (sR == sectorIdx) neighborIdx = sL;
        else if (sL == sectorIdx) neighborIdx = sR;

        if (neighborIdx != -1) {
            float nFloor = (float)mSectors[neighborIdx].floorHeight;
            if (first || nFloor < lowest) {
                lowest = nFloor;
                first = false;
            }
        }
    }
    return lowest;
}

std::vector<int> Map::GetSectorsByTag(int tag) const {
    std::vector<int> result;
    if (tag == 0) return result;
    for (int i = 0; i < (int)mSectors.size(); ++i) {
        if (mSectors[i].tag == tag) {
            result.push_back(i);
        }
    }
    return result;
}

