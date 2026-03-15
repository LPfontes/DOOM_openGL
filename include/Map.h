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
    const std::vector<WADSubSector>& GetSubSectors() const { return mSubSectors; }
    const std::vector<WADSeg>& GetSegs() const { return mSegs; }
    const std::vector<WADThing>& GetThings() const { return mThings; }

private:
    std::string mName;
    std::vector<WADVertex> mVertices;
    std::vector<WADLineDef> mLineDefs;
    std::vector<WADSideDef> mSideDefs;
    std::vector<WADSector> mSectors;
    std::vector<WADSubSector> mSubSectors;
    std::vector<WADSeg> mSegs;
    std::vector<WADThing> mThings;
};



#endif // MAP_H
