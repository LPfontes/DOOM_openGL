#include "Scene.h"
#include <iostream>

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

    for (const auto& line : lineDefs) {
        if (line.v1 < 0 || line.v2 < 0) continue;

        // Get vertices
        const auto& v1 = mapVertices[line.v1];
        const auto& v2 = mapVertices[line.v2];

        // Let's take the right sidedef to get sector info
        if (line.rightSideDef == -1) continue;
        const auto& side = sideDefs[line.rightSideDef];
        if (side.sector < 0) continue;
        const auto& sector = sectors[side.sector];

        float floor = (float)sector.floorHeight;
        float ceil = (float)sector.ceilingHeight;

        // Create a wall (Quad = 2 Triangles)
        // Triangle 1
        float r = 0.5f, g = 0.5f, b = 0.5f; // Gray wall
        
        vertices.push_back({(float)v1.x, floor, (float)v1.y, r, g, b});
        vertices.push_back({(float)v2.x, floor, (float)v2.y, r, g, b});
        vertices.push_back({(float)v1.x, ceil, (float)v1.y, r, g, b});

        // Triangle 2
        vertices.push_back({(float)v2.x, floor, (float)v2.y, r, g, b});
        vertices.push_back({(float)v2.x, ceil,  (float)v2.y, r, g, b});
        vertices.push_back({(float)v1.x, ceil,  (float)v1.y, r, g, b});
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
