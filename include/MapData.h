#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push, 1)

// Coisa (Thing) do Doom
struct WADThing {
    int16_t x;
    int16_t y;
    int16_t angle;
    int16_t type;
    int16_t options;
};

// Vértice (Vertex) do Doom
struct WADVertex {
    int16_t x;
    int16_t y;
};

// Linedef do Doom (v1 e v2 são índices para vértices)
struct WADLineDef {
    int16_t v1;
    int16_t v2;
    int16_t flags;
    int16_t specialType;
    int16_t sectorTag;
    int16_t rightSideDef;
    int16_t leftSideDef;
};

// Sidedef do Doom
struct WADSideDef {
    int16_t xOffset;
    int16_t yOffset;
    char upperTexture[8];
    char lowerTexture[8];
    char middleTexture[8];
    int16_t sector;
};

// Setor (Sector) do Doom
struct WADSector {
    int16_t floorHeight;
    int16_t ceilingHeight;
    char floorTexture[8];
    char ceilingTexture[8];
    int16_t lightLevel;
    int16_t specialType;
    int16_t tag;
};

// Seg do Doom (parte de um sub-setor)
struct WADSeg {
    int16_t v1;
    int16_t v2;
    int16_t angle;
    int16_t lineDef;
    int16_t side;
    int16_t offset;
};

// Sub-setor (SubSector) do Doom
struct WADSubSector {
    int16_t numSegs;
    int16_t firstSeg;
};

// Nó (Node) do Doom (para BSP)
struct WADNode {
    int16_t x, y, dx, dy; // Linha de partição
    int16_t bbox[2][4];   // [right, left][top, bottom, left, right]
    uint16_t children[2]; // Se o bit 15 for 1, é um sub-setor (índice com o bit 15 limpo)
};

#pragma pack(pop)


#endif // MAP_DATA_H
