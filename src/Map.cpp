#include "Map.h"
#include <iostream>
#include <cstring>

Map::Map(const std::string& name) : mName(name) {}

bool Map::LoadFromWAD(WADParser& parser) {
    int mapIndex = parser.FindLump(mName);
    if (mapIndex == -1) {
        std::cerr << "Map " << mName << " not found in WAD." << std::endl;
        return false;
    }

    // Doom maps have a specific order of lumps following the map name
    // 1: THINGS, 2: LINEDEFS, 3: SIDEDEFS, 4: VERTEXES, 5: SEGS, 6: SSECTORS, 7: NODES, 8: SECTORS, 9: REJECT, 10: BLOCKMAP
    
    // Load Things (Index + 1)
    auto thingData = parser.ReadLumpData(mapIndex + 1);
    int numThings = thingData.size() / sizeof(WADThing);
    mThings.resize(numThings);
    memcpy(mThings.data(), thingData.data(), thingData.size());

    // Load Vertices (Index + 4)
    auto vertexData = parser.ReadLumpData(mapIndex + 4);

    int numVertices = vertexData.size() / sizeof(WADVertex);
    mVertices.resize(numVertices);
    memcpy(mVertices.data(), vertexData.data(), vertexData.size());

    // Load LineDefs (Index + 2)
    auto lineData = parser.ReadLumpData(mapIndex + 2);
    int numLines = lineData.size() / sizeof(WADLineDef);
    mLineDefs.resize(numLines);
    memcpy(mLineDefs.data(), lineData.data(), lineData.size());

    // Load SideDefs (Index + 3)
    auto sideData = parser.ReadLumpData(mapIndex + 3);
    int numSides = sideData.size() / sizeof(WADSideDef);
    mSideDefs.resize(numSides);
    memcpy(mSideDefs.data(), sideData.data(), sideData.size());

    // Load Sectors (Index + 8)
    auto sectorData = parser.ReadLumpData(mapIndex + 8);
    int numSectors = sectorData.size() / sizeof(WADSector);
    mSectors.resize(numSectors);
    memcpy(mSectors.data(), sectorData.data(), sectorData.size());

    // Load Segs (Index + 5)
    auto segData = parser.ReadLumpData(mapIndex + 5);
    int numSegs = segData.size() / sizeof(WADSeg);
    mSegs.resize(numSegs);
    memcpy(mSegs.data(), segData.data(), segData.size());

    // Load SubSectors (Index + 6)
    auto ssectorData = parser.ReadLumpData(mapIndex + 6);
    int numSSectors = ssectorData.size() / sizeof(WADSubSector);
    mSubSectors.resize(numSSectors);
    memcpy(mSubSectors.data(), ssectorData.data(), ssectorData.size());

    std::cout << "Loaded map: " << mName << std::endl;

    std::cout << "Vertices: " << mVertices.size() << std::endl;
    std::cout << "LineDefs: " << mLineDefs.size() << std::endl;
    std::cout << "Sectors:  " << mSectors.size() << std::endl;

    return true;
}
