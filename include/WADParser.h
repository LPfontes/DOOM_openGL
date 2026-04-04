#ifndef WAD_PARSER_H
#define WAD_PARSER_H

#include <string>
#include <vector>
#include <array>
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

    // Texture decoding
    // Returns raw RGB bytes (64*64*3) for a flat (floor/ceiling texture)
    std::vector<uint8_t> GetFlatRGB(const std::string& name);
    // Returns raw RGB bytes (outW*outH*3) for a composite wall texture
    std::vector<uint8_t> GetWallTextureRGB(const std::string& name, int& outW, int& outH);

private:
    std::string mFilename;
    std::ifstream mFile;
    WADHeader mHeader;
    std::vector<WADLumpEntry> mLumps;

    // Lazy-loaded palette and patch names
    std::array<uint8_t, 768> mPalette;
    bool mPaletteParsed = false;
    std::vector<std::string> mPatchNames;
    bool mPatchNamesParsed = false;

    void EnsurePalette();
    void EnsurePatchNames();

    // Draws a patch (picture format) into an RGB output buffer at (destX, destY)
    void DecodePatch(const std::vector<uint8_t>& patchData,
                     std::vector<uint8_t>& output,
                     int texW, int texH,
                     int destX, int destY);
};

#endif // WAD_PARSER_H
