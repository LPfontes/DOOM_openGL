#ifndef WAD_PARSER_H
#define WAD_PARSER_H

#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <cstdint>

#pragma pack(push, 1)
struct WADHeader {
    char type[4];          // "IWAD" ou "PWAD"
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

    // Encontra o índice de um lump pelo nome
    int FindLump(const std::string& name, int startIndex = 0);

    // Lê dados brutos de um lump
    std::vector<uint8_t> ReadLumpData(const WADLumpEntry& entry);
    std::vector<uint8_t> ReadLumpData(int index);

    // Decodificação de textura
    // Retorna bytes RGB brutos (64*64*3) para um flat (textura de piso/teto)
    std::vector<uint8_t> GetFlatRGB(const std::string& name);
    // Retorna bytes RGB brutos (outW*outH*3) para uma textura de parede composta
    std::vector<uint8_t> GetWallTextureRGB(const std::string& name, int& outW, int& outH);

private:
    std::string mFilename;
    std::ifstream mFile;
    WADHeader mHeader;
    std::vector<WADLumpEntry> mLumps;

    // Paleta e nomes de patches carregados sob demanda (lazy-loaded)
    std::array<uint8_t, 768> mPalette;
    bool mPaletteParsed = false;
    std::vector<std::string> mPatchNames;
    bool mPatchNamesParsed = false;

    void EnsurePalette();
    void EnsurePatchNames();

    // Desenha um patch (formato de imagem) em um buffer de saída RGB em (destX, destY)
    void DecodePatch(const std::vector<uint8_t>& patchData,
                     std::vector<uint8_t>& output,
                     int texW, int texH,
                     int destX, int destY);
};

#endif // WAD_PARSER_H
