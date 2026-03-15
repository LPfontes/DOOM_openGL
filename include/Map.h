#ifndef MAP_H
#define MAP_H

#include "MapData.h"
#include "WADParser.h"
#include <vector>
#include <string>

class Map {
public:
    Map(const std::string& name);
    
    bool LoadFromWAD(WADParser& parser);
    
    const std::vector<WADVertex>& GetVertices() const { return mVertices; }
    const std::vector<WADLineDef>& GetLineDefs() const { return mLineDefs; }
    const std::vector<WADSideDef>& GetSideDefs() const { return mSideDefs; }
    const std::vector<WADSector>& GetSectors() const { return mSectors; }

private:
    std::string mName;
    std::vector<WADVertex> mVertices;
    std::vector<WADLineDef> mLineDefs;
    std::vector<WADSideDef> mSideDefs;
    std::vector<WADSector> mSectors;
};

#endif // MAP_H
