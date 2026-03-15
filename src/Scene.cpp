#include "Scene.h"
#include <iostream>
#include <glm/glm.hpp>

glm::vec3 getColorForHeight(float height) {
    if (height < 0.0f) return glm::vec3(0.2f, 0.4f, 0.8f); // Azul para níveis baixos
    if (height == 0.0f) return glm::vec3(0.2f, 0.7f, 0.3f); // Verde para o nível base
    if (height < 100.0f) return glm::vec3(0.8f, 0.3f, 0.2f); // Vermelho para níveis médios
    return glm::vec3(0.8f, 0.7f, 0.2f); // Amarelo para níveis altos
}

Scene::Scene() : mVAO(0), mVBO(0), mVertexCount(0) {}

Scene::~Scene() {
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
}

void Scene::GenerateFromMap(const Map& map) {
    std::vector<Vertex3D> vertices;

    const auto& mapVertices = map.GetVertices();
    const auto& lineDefs = map.GetLineDefs();
    const auto& sideDefs = map.GetSideDefs();
    const auto& sectors = map.GetSectors();
    const auto& segs = map.GetSegs();
    const auto& ssectors = map.GetSubSectors();

    auto addWall = [&](const WADVertex& vertex1, const WADVertex& vertex2, float bottom, float top, float r, float g, float b) {
        if (top <= bottom) return;
        // Triangle 1
        vertices.push_back({(float)vertex1.x, bottom, (float)vertex1.y, r, g, b});
        vertices.push_back({(float)vertex2.x, bottom, (float)vertex2.y, r, g, b});
        vertices.push_back({(float)vertex1.x, top,    (float)vertex1.y, r, g, b});
        // Triangle 2
        vertices.push_back({(float)vertex2.x, bottom, (float)vertex2.y, r, g, b});
        vertices.push_back({(float)vertex2.x, top,    (float)vertex2.y, r, g, b});
        vertices.push_back({(float)vertex1.x, top,    (float)vertex1.y, r, g, b});
    };

    // --- Render Walls via SEGS ---
    // Segs are better because they are already clipped and belong to one side
    for (const auto& seg : segs) {
        if (seg.lineDef == -1) continue; // Skip minisegs for walls

        const auto& line = lineDefs[seg.lineDef];
        const auto& v1 = mapVertices[seg.v1];
        const auto& v2 = mapVertices[seg.v2];

        int sideIdx = (seg.side == 0) ? line.rightSideDef : line.leftSideDef;
        if (sideIdx == -1) continue;
        const auto& side = sideDefs[sideIdx];
        const auto& sector = sectors[side.sector];
        float light = (float)sector.lightLevel / 255.0f;
        glm::vec3 baseColor = getColorForHeight((float)sector.floorHeight);

        if (line.leftSideDef == -1) {
            // Solid Wall
            addWall(v1, v2, (float)sector.floorHeight, (float)sector.ceilingHeight, light * baseColor.r, light * baseColor.g, light * baseColor.b);
        } else {
            // 2-Sided Line (Portal)
            int otherSideIdx = (seg.side == 0) ? line.leftSideDef : line.rightSideDef;
            const auto& otherSide = sideDefs[otherSideIdx];
            const auto& otherSector = sectors[otherSide.sector];

            // Lower Wall
            if (otherSector.floorHeight > sector.floorHeight) {
                glm::vec3 col = getColorForHeight((float)otherSector.floorHeight);
                addWall(v1, v2, (float)sector.floorHeight, (float)otherSector.floorHeight, light * col.r * 0.8f, light * col.g * 0.8f, light * col.b * 0.8f);
            }
            // Upper Wall
            if (otherSector.ceilingHeight < sector.ceilingHeight) {
                glm::vec3 col = getColorForHeight((float)sector.ceilingHeight);
                addWall(v1, v2, (float)otherSector.ceilingHeight, (float)sector.ceilingHeight, light * col.r * 0.9f, light * col.g * 0.9f, light * col.b * 0.9f);
            }
        }
    }

    // --- Render Floors and Ceilings via Ear Clipping (APPROACH 2) ---
    for (int sIdx = 0; sIdx < sectors.size(); ++sIdx) {
        const auto& sector = sectors[sIdx];
        float floor = (float)sector.floorHeight;
        float ceil = (float)sector.ceilingHeight;
        float light = (float)sector.lightLevel / 255.0f;
        glm::vec3 fCol = getColorForHeight(floor) * light * 0.7f;
        glm::vec3 cCol = getColorForHeight(ceil) * light * 1.1f;

        // 1. Collect all linedefs belonging to this sector
        struct Edge { int v1, v2; };
        std::vector<Edge> edges;
        for (const auto& line : lineDefs) {
            if (line.rightSideDef != -1 && sideDefs[line.rightSideDef].sector == sIdx) {
                edges.push_back({line.v1, line.v2});
            }
            if (line.leftSideDef != -1 && sideDefs[line.leftSideDef].sector == sIdx) {
                edges.push_back({line.v2, line.v1}); // Reverse for left side
            }
        }

        if (edges.empty()) continue;

        // 2. Group edges into closed loops
        std::vector<std::vector<int>> loops;
        while (!edges.empty()) {
            std::vector<int> loop;
            loop.push_back(edges[0].v1);
            int currentV = edges[0].v2;
            edges.erase(edges.begin());

            bool foundNext = true;
            while (foundNext && currentV != loop[0]) {
                foundNext = false;
                for (size_t i = 0; i < edges.size(); ++i) {
                    if (edges[i].v1 == currentV) {
                        loop.push_back(currentV);
                        currentV = edges[i].v2;
                        edges.erase(edges.begin() + i);
                        foundNext = true;
                        break;
                    }
                }
            }
            if (loop.size() >= 3) loops.push_back(loop);
        }

        // 3. Triangulate each loop (Simple Ear Clipping for each disconnected part)
        // Note: This simplified version doesn't handle holes (inner sectors) perfectly,
        // but for Doom's planar-like structure, rendering loops individually often works.
        for (auto& loop : loops) {
            std::vector<int> workingLoop = loop;
            while (workingLoop.size() >= 3) {
                bool earFound = false;
                for (size_t i = 0; i < workingLoop.size(); ++i) {
                    int i0 = (i == 0) ? workingLoop.size() - 1 : i - 1;
                    int i1 = i;
                    int i2 = (i == workingLoop.size() - 1) ? 0 : i + 1;

                    const auto& v0 = mapVertices[workingLoop[i0]];
                    const auto& v1 = mapVertices[workingLoop[i1]];
                    const auto& v2 = mapVertices[workingLoop[i2]];

                    // Check if triangle v0-v1-v2 is an "ear" (convex and no other points inside)
                    // Simplified: just check convexity for now (Doom sectors are often "good")
                    float cross = (float)(v1.x - v0.x) * (v2.y - v1.y) - (float)(v1.y - v0.y) * (v2.x - v1.x);
                    
                    if (cross < 0) { // Convex for clockwise (Doom standard is often CW for sector loops)
                        // Floor
                        vertices.push_back({(float)v0.x, floor, (float)v0.y, fCol.r, fCol.g, fCol.b});
                        vertices.push_back({(float)v1.x, floor, (float)v1.y, fCol.r, fCol.g, fCol.b});
                        vertices.push_back({(float)v2.x, floor, (float)v2.y, fCol.r, fCol.g, fCol.b});

                        // Ceiling (Swap v1/v2)
                        vertices.push_back({(float)v0.x, ceil, (float)v0.y, cCol.r, cCol.g, cCol.b});
                        vertices.push_back({(float)v2.x, ceil, (float)v2.y, cCol.r, cCol.g, cCol.b});
                        vertices.push_back({(float)v1.x, ceil, (float)v1.y, cCol.r, cCol.g, cCol.b});

                        workingLoop.erase(workingLoop.begin() + i1);
                        earFound = true;
                        break;
                    }
                }
                if (!earFound) break; // Avoid infinite loop on messy geometry
            }
        }
    }


    mVertexCount = vertices.size();

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Scene::Render() {
    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLES, 0, mVertexCount);
}
