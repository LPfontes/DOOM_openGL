#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push, 1)

// Doom Thing
struct WADThing {
    int16_t x;
    int16_t y;
    int16_t angle;
    int16_t type;
    int16_t options;
};

// Doom Vertex
struct WADVertex {
    int16_t x;
    int16_t y;
};

// Doom Linedef (v1 and v2 are indices to vertexes)
struct WADLineDef {
    int16_t v1;
    int16_t v2;
    int16_t flags;
    int16_t specialType;
    int16_t sectorTag;
    int16_t rightSideDef;
    int16_t leftSideDef;
};

// Doom Sidedef
struct WADSideDef {
    int16_t xOffset;
    int16_t yOffset;
    char upperTexture[8];
    char lowerTexture[8];
    char middleTexture[8];
    int16_t sector;
};

// Doom Sector
struct WADSector {
    int16_t floorHeight;
    int16_t ceilingHeight;
    char floorTexture[8];
    char ceilingTexture[8];
    int16_t lightLevel;
    int16_t specialType;
    int16_t tag;
};

// Doom Seg (part of a subsector)
struct WADSeg {
    int16_t v1;
    int16_t v2;
    int16_t angle;
    int16_t lineDef;
    int16_t side;
    int16_t offset;
};

// Doom SubSector
struct WADSubSector {
    int16_t numSegs;
    int16_t firstSeg;
};

#pragma pack(pop)


#endif // MAP_DATA_H
