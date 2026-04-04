#include "WADParser.h"
#include <iostream>
#include <cstring>
#include <cctype>

WADParser::WADParser(const std::string& filename) : mFilename(filename) {}

WADParser::~WADParser() {
    if (mFile.is_open()) {
        mFile.close();
    }
}

bool WADParser::Load() {
    mFile.open(mFilename, std::ios::binary);
    if (!mFile.is_open()) {
        std::cerr << "Could not open WAD file: " << mFilename << std::endl;
        return false;
    }

    // Read Header
    mFile.read((char*)&mHeader, sizeof(WADHeader));
    
    std::string type(mHeader.type, 4);
    if (type != "IWAD" && type != "PWAD") {
        std::cerr << "Invalid WAD type: " << type << std::endl;
        return false;
    }

    // Read Directory
    mFile.seekg(mHeader.directoryAddress);
    mLumps.resize(mHeader.numLumps);
    for (uint32_t i = 0; i < mHeader.numLumps; ++i) {
        mFile.read((char*)&mLumps[i], sizeof(WADLumpEntry));
    }

    std::cout << "Successfully loaded WAD: " << mFilename << std::endl;
    std::cout << "Lumps: " << mHeader.numLumps << std::endl;

    return true;
}

int WADParser::FindLump(const std::string& name, int startIndex) {
    char cleanName[9] = {0};
    strncpy(cleanName, name.c_str(), 8);

    for (int i = startIndex; i < (int)mLumps.size(); ++i) {
        if (strncasecmp(mLumps[i].name, cleanName, 8) == 0) {
            return i;
        }
    }
    return -1;
}

std::vector<uint8_t> WADParser::ReadLumpData(const WADLumpEntry& entry) {
    std::vector<uint8_t> data(entry.size);
    mFile.clear(); // reset eofbit/failbit before seeking
    mFile.seekg(entry.filePos);
    mFile.read((char*)data.data(), entry.size);
    return data;
}

std::vector<uint8_t> WADParser::ReadLumpData(int index) {
    if (index < 0 || index >= (int)mLumps.size()) return {};
    return ReadLumpData(mLumps[index]);
}

// --- Texture helpers ---

void WADParser::EnsurePalette() {
    if (mPaletteParsed) return;
    mPaletteParsed = true;
    mPalette.fill(0);
    int idx = FindLump("PLAYPAL");
    if (idx == -1) return;
    auto data = ReadLumpData(idx);
    // First palette: 256 RGB entries = 768 bytes
    for (int i = 0; i < 768 && i < (int)data.size(); ++i)
        mPalette[i] = data[i];
}

void WADParser::EnsurePatchNames() {
    if (mPatchNamesParsed) return;
    mPatchNamesParsed = true;
    int idx = FindLump("PNAMES");
    if (idx == -1) return;
    auto data = ReadLumpData(idx);
    if (data.size() < 4) return;
    uint32_t count;
    memcpy(&count, data.data(), 4);
    mPatchNames.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        size_t off = 4 + i * 8;
        if (off + 8 > data.size()) break;
        char buf[9] = {0};
        memcpy(buf, data.data() + off, 8);
        // uppercase for lookup
        for (int c = 0; c < 8; ++c)
            buf[c] = (char)toupper((unsigned char)buf[c]);
        mPatchNames.push_back(std::string(buf));
    }
}

void WADParser::DecodePatch(const std::vector<uint8_t>& data,
                             std::vector<uint8_t>& output,
                             int texW, int texH,
                             int destX, int destY)
{
    if (data.size() < 8) return;
    int16_t patchW;
    memcpy(&patchW, data.data(), 2);

    for (int col = 0; col < (int)patchW; ++col) {
        int px = destX + col;
        if (px < 0 || px >= texW) continue;

        size_t colOffPos = 8 + col * 4;
        if (colOffPos + 4 > data.size()) break;
        uint32_t colOff;
        memcpy(&colOff, data.data() + colOffPos, 4);
        if (colOff >= data.size()) continue;

        size_t pos = colOff;
        while (pos < data.size()) {
            uint8_t topdelta = data[pos++];
            if (topdelta == 0xFF) break;
            if (pos >= data.size()) break;
            uint8_t length = data[pos++];
            pos++; // skip junk byte
            for (int i = 0; i < (int)length; ++i) {
                if (pos >= data.size()) break;
                int py = (int)topdelta + i + destY;
                uint8_t palIdx = data[pos++];
                if (py >= 0 && py < texH && px >= 0 && px < texW) {
                    int pixIdx = (py * texW + px) * 3;
                    output[pixIdx + 0] = mPalette[palIdx * 3 + 0];
                    output[pixIdx + 1] = mPalette[palIdx * 3 + 1];
                    output[pixIdx + 2] = mPalette[palIdx * 3 + 2];
                }
            }
            pos++; // skip trailing junk byte
        }
    }
}

std::vector<uint8_t> WADParser::GetFlatRGB(const std::string& name) {
    EnsurePalette();
    // Flats live between F_START and F_END markers in the WAD directory
    int fstart = FindLump("F_START");
    int fend   = FindLump("F_END");
    if (fstart == -1) fstart = FindLump("FF_START");
    if (fend   == -1) fend   = FindLump("FF_END");
    int idx = (fstart != -1 && fend != -1)
              ? FindLump(name, fstart + 1)
              : FindLump(name);
    if (idx == -1 || (fend != -1 && idx >= fend)) return {};
    auto data = ReadLumpData(idx);
    if (data.size() < 64 * 64) return {};
    std::vector<uint8_t> rgb(64 * 64 * 3);
    for (int i = 0; i < 64 * 64; ++i) {
        uint8_t palIdx = data[i];
        rgb[i * 3 + 0] = mPalette[palIdx * 3 + 0];
        rgb[i * 3 + 1] = mPalette[palIdx * 3 + 1];
        rgb[i * 3 + 2] = mPalette[palIdx * 3 + 2];
    }
    return rgb;
}

std::vector<uint8_t> WADParser::GetWallTextureRGB(const std::string& name, int& outW, int& outH) {
    EnsurePalette();
    EnsurePatchNames();

    // Normalize name to uppercase, max 8 chars
    std::string uname = name.substr(0, 8);
    for (auto& c : uname) c = (char)toupper((unsigned char)c);

    // Search TEXTURE1 and TEXTURE2
    for (const char* lumpName : {"TEXTURE1", "TEXTURE2"}) {
        int texLumpIdx = FindLump(lumpName);
        if (texLumpIdx == -1) continue;
        auto texData = ReadLumpData(texLumpIdx);
        if (texData.size() < 4) continue;

        uint32_t numTex;
        memcpy(&numTex, texData.data(), 4);

        for (uint32_t t = 0; t < numTex; ++t) {
            size_t offPos = 4 + t * 4;
            if (offPos + 4 > texData.size()) break;
            uint32_t offset;
            memcpy(&offset, texData.data() + offPos, 4);
            if (offset + 22 > texData.size()) continue;

            const uint8_t* tp = texData.data() + offset;

            // Compare name (8 chars, uppercase)
            char tname[9] = {0};
            memcpy(tname, tp, 8);
            for (int c = 0; c < 8; ++c)
                tname[c] = (char)toupper((unsigned char)tname[c]);
            if (strncmp(tname, uname.c_str(), 8) != 0) continue;

            // Found it
            // Doom texture struct: name[8], masked(int=4), width(2), height(2),
            // columndirectory(int=4), patchcount(2) — total 22 bytes header
            int16_t width, height;
            memcpy(&width,  tp + 12, 2);
            memcpy(&height, tp + 14, 2);
            uint16_t patchCount;
            memcpy(&patchCount, tp + 20, 2);

            outW = width;
            outH = height;
            // Initialize RGB to dark gray (missing pixels fallback)
            std::vector<uint8_t> rgb(width * height * 3, 64);

            const uint8_t* pp = tp + 22;
            for (int p = 0; p < (int)patchCount; ++p) {
                if (pp + 10 > texData.data() + texData.size()) break;
                int16_t originX, originY;
                uint16_t patchIdx;
                memcpy(&originX,   pp + 0, 2);
                memcpy(&originY,   pp + 2, 2);
                memcpy(&patchIdx,  pp + 4, 2);
                pp += 10;

                if ((int)patchIdx >= (int)mPatchNames.size()) continue;
                int patchLump = FindLump(mPatchNames[patchIdx]);
                if (patchLump == -1) continue;
                auto patchData = ReadLumpData(patchLump);
                DecodePatch(patchData, rgb, width, height, originX, originY);
            }
            return rgb;
        }
    }
    return {};
}
