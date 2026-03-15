#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>

struct WADHeader {
    char identification[4];
    int numlumps;
    int infotableofs;
};

struct WADEntry {
    int filepos;
    int size;
    char name[8];
};

int main() {
    std::ifstream file("assets/doom1.wad", std::ios::binary);
    if (!file) return 1;

    WADHeader header;
    file.read((char*)&header, sizeof(header));

    file.seekg(header.infotableofs);
    for (int i = 0; i < header.numlumps; ++i) {
        WADEntry entry;
        file.read((char*)&entry, sizeof(entry));
        char name[9];
        std::memcpy(name, entry.name, 8);
        name[8] = '\0';
        if (std::string(name).find("GL_") == 0) {
            std::cout << "Found: " << name << " at index " << i << std::endl;
        }
    }
    return 0;
}
