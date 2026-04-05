#ifndef MAP_H
#define MAP_H

#include "MapData.h"
#include "WADParser.h"
#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>

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
    const std::vector<WADNode>& GetNodes() const { return mNodes; }

    int GetSectorAt(float x, float y) const;
    void ToggleDoor(int lineDefIdx);
    bool IsDoorOpen(int lineDefIdx) const;
    int RayCastToLineDef(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const;
    
    std::vector<float>& GetCeilOffsets() { return mCeilOffsets; }
    std::vector<float>& GetFloorOffsets() { return mFloorOffsets; }

private:
    std::string mName;
    std::vector<WADVertex> mVertices;
    std::vector<WADLineDef> mLineDefs;
    std::vector<WADSideDef> mSideDefs;
    std::vector<WADSector> mSectors;
    std::vector<WADSubSector> mSubSectors;
    std::vector<WADSeg> mSegs;
    std::vector<WADThing> mThings;
    std::vector<WADNode> mNodes;

    std::vector<float> mCeilOffsets;
    std::vector<float> mFloorOffsets;

    std::map<int, bool> mDoorStates;  // linedef index -> aberta/fechada
};



#endif // MAP_H
