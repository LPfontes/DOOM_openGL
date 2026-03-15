#include "WADParser.h"
#include <iostream>
#include <cstring>

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
    mFile.seekg(entry.filePos);
    mFile.read((char*)data.data(), entry.size);
    return data;
}

std::vector<uint8_t> WADParser::ReadLumpData(int index) {
    if (index < 0 || index >= (int)mLumps.size()) return {};
    return ReadLumpData(mLumps[index]);
}
