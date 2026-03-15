#ifndef WAD_PARSER_H
#define WAD_PARSER_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

#pragma pack(push, 1)
struct WADHeader {
    char type[4];          // "IWAD" or "PWAD"
    uint32_t numLumps;
    uint32_t directoryAddress;
};

struct WADLumpEntry {
    uint32_t filePos;
    uint32_t size;
    char name[8];
};
#pragma pack(pop)

class WADParser {
public:
    WADParser(const std::string& filename);
    ~WADParser();

    bool Load();
    const std::vector<WADLumpEntry>& GetLumps() const { return mLumps; }
    
    // Finds a lump index by name
    int FindLump(const std::string& name, int startIndex = 0);
    
    // Reads raw data from a lump
    std::vector<uint8_t> ReadLumpData(const WADLumpEntry& entry);
    std::vector<uint8_t> ReadLumpData(int index);

private:
    std::string mFilename;
    std::ifstream mFile;
    WADHeader mHeader;
    std::vector<WADLumpEntry> mLumps;
};

#endif // WAD_PARSER_H
