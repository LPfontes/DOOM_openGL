#include "Scene.h"
#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <mapbox/earcut.hpp>

Scene::Scene() {}

Scene::~Scene() {
    for (auto& b : mBatches) {
        glDeleteVertexArrays(1, &b.vao);
        glDeleteBuffers(1, &b.vbo);
    }
    for (auto& kv : mTexCache)
        glDeleteTextures(1, &kv.second.id);
    if (mFallbackTex)
        glDeleteTextures(1, &mFallbackTex);
}

GLuint Scene::CreateGLTexture(const std::vector<uint8_t>& rgb, int w, int h) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static std::string NormalizeName(const char* name8) {
    char buf[9] = {0};
    strncpy(buf, name8, 8);
    return std::string(buf); // para no primeiro nulo
}

TexEntry Scene::GetOrLoadWallTex(const char* name8, WADParser& wad) {
    std::string name = NormalizeName(name8);
    if (name.empty() || name == "-")
        return {mFallbackTex, 64, 64};

    auto it = mTexCache.find(name);
    if (it != mTexCache.end()) return it->second;

    int w = 0, h = 0;
    auto rgb = wad.GetWallTextureRGB(name, w, h);
    if (rgb.empty() || w == 0 || h == 0) {
        mTexCache[name] = {mFallbackTex, 64, 64};
        return {mFallbackTex, 64, 64};
    }
    TexEntry entry{CreateGLTexture(rgb, w, h), w, h};
    mTexCache[name] = entry;
    return entry;
}

TexEntry Scene::GetOrLoadFlatTex(const char* name8, WADParser& wad) {
    std::string name = NormalizeName(name8);
    if (name.empty() || name == "-")
        return {mFallbackTex, 64, 64};

    auto it = mTexCache.find(name);
    if (it != mTexCache.end()) return it->second;

    auto rgb = wad.GetFlatRGB(name);
    if (rgb.empty()) {
        mTexCache[name] = {mFallbackTex, 64, 64};
        return {mFallbackTex, 64, 64};
    }
    TexEntry entry{CreateGLTexture(rgb, 64, 64), 64, 64};
    mTexCache[name] = entry;
    return entry;
}

void Scene::GenerateFromMap(const Map& map, WADParser& wad) {
    // Inicializa o Ray Tracer
    mRTManager.Init();
    // Atualiza os dados do mapa
    mRTManager.UpdateMapData(map.GetLineDefs(), map.GetVertices(), map.GetSectors(), map.GetSideDefs());
    // Atualiza as luzes
    mRTManager.UpdateLights(map.GetThings(), map.GetSectors(), 0); 

    // Cria um padrão xadrez (checkerboard) magenta/preto 8x8 de fallback
    {
        std::vector<uint8_t> fb(8 * 8 * 3);
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x) {
                bool chk = (x + y) % 2 == 0;
                int i = (y * 8 + x) * 3;
                fb[i+0] = chk ? 255 : 0;
                fb[i+1] = 0;
                fb[i+2] = chk ? 255 : 0;
            }
        mFallbackTex = CreateGLTexture(fb, 8, 8);
    }
    // Pega os dados do mapa
    const auto& mapVertices = map.GetVertices();
    const auto& lineDefs    = map.GetLineDefs();
    const auto& sideDefs    = map.GetSideDefs();
    const auto& sectors     = map.GetSectors();
    const auto& segs        = map.GetSegs();

    // Mapa: texId -> lista de vértices
    std::map<GLuint, std::vector<Vertex3D>> batchMap;

    // Auxiliar: adiciona um quadrilátero de parede (quad) ao lote (batch) correto
    auto addWall = [&](const WADVertex& va, const WADVertex& vb,
                        float bottom, float top,
                        int sIdxBot, int sIdxTop,
                        float segOffset, float xOff, float yOff,
                        float light, const TexEntry& tex,
                        float vTypeBot, float vTypeTop)
    {
        // Se o topo for menor que a base, não desenha
        if (top <= bottom) return;
        // Calcula o comprimento do segmento pelo Teorema de Pitágoras
        float segLen = sqrtf( 
            (float)(vb.x - va.x) * (vb.x - va.x) +
            (float)(vb.y - va.y) * (vb.y - va.y));
        // Calcula a altura da parede
        float wallH = top - bottom;
        // Calcula as coordenadas de textura
        float u0 = (segOffset + xOff) / (float)tex.w;
        float u1 = (segOffset + xOff + segLen) / (float)tex.w;
        float v_top = yOff / (float)tex.h;
        float v_bot = (wallH + yOff) / (float)tex.h;
        // Índices dos setores
        float siBot = (float)sIdxBot;
        float siTop = (float)sIdxTop;
        // Adiciona os vértices ao lote
        auto& verts = batchMap[tex.id];
        float invH = 1.0f / (float)tex.h;
        // Triângulo 1
        // va.x, bottom, va.y: Posição 3D do vértice y vira z no OpenGL
        verts.push_back({(float)va.x, bottom, (float)va.y, u0, v_bot, light, light, light, siBot, vTypeBot, invH});
        verts.push_back({(float)vb.x, bottom, (float)vb.y, u1, v_bot, light, light, light, siBot, vTypeBot, invH});
        verts.push_back({(float)va.x, top,    (float)va.y, u0, v_top, light, light, light, siTop, vTypeTop, invH});
        // Triângulo 2
        verts.push_back({(float)vb.x, bottom, (float)vb.y, u1, v_bot, light, light, light, siBot, vTypeBot, invH});
        verts.push_back({(float)vb.x, top,    (float)vb.y, u1, v_top, light, light, light, siTop, vTypeTop, invH});
        verts.push_back({(float)va.x, top,    (float)va.y, u0, v_top, light, light, light, siTop, vTypeTop, invH});
    };

    // --- Paredes via SEGS ---
    for (const auto& seg : segs) {
        // Se o lineDef for -1, não desenha
        if (seg.lineDef == -1) continue;
        // Se a porta estiver aberta, não desenha
        if (map.IsDoorOpen(seg.lineDef)) continue;
        // Pega os dados do segmento
        const auto& line = lineDefs[seg.lineDef];
        const auto& v1   = mapVertices[seg.v1];
        const auto& v2   = mapVertices[seg.v2];
        // Pega o índice do lado
        int sideIdx = (seg.side == 0) ? line.rightSideDef : line.leftSideDef;
        if (sideIdx == -1) continue;
        // Pega os dados do lado
        const auto& side   = sideDefs[sideIdx];
        const auto& sector = sectors[side.sector];
        // Calcula a iluminação
        float light        = (float)sector.lightLevel / 255.0f;
        float sOff         = (float)seg.offset;
        float xOff         = (float)side.xOffset;
        float yOff         = (float)side.yOffset;

        if (line.leftSideDef == -1) { 
            // Parede sólida — usa textura do meio (middle texture)
            auto tex = GetOrLoadWallTex(side.middleTexture, wad);
            addWall(v1, v2,
                    (float)sector.floorHeight, (float)sector.ceilingHeight,
                    side.sector, side.sector,
                    sOff, xOff, yOff, light, tex,
                    3.0f, 2.0f); // Padrão: Base segue o Piso, Topo segue o Teto
        } else {
            // Portal — pode ter segmentos de parede inferiores e superiores
            int otherSideIdx = (seg.side == 0) ? line.leftSideDef : line.rightSideDef;
            const auto& otherSide   = sideDefs[otherSideIdx];
            const auto& otherSector = sectors[otherSide.sector];

            // Parede inferior (degrau/step up)
            if (otherSector.floorHeight > sector.floorHeight) {
                std::string ltex = NormalizeName(side.lowerTexture);
                // Se a textura não for vazia
                if (ltex != "-" && !ltex.empty()) {
                    auto tex = GetOrLoadWallTex(side.lowerTexture, wad);
                    // Adiciona o quadrilátero de parede inferior
                    addWall(v1, v2,
                            (float)sector.floorHeight, (float)otherSector.floorHeight,
                            side.sector, otherSide.sector,
                            sOff, xOff, yOff, light, tex,
                            3.0f, 5.0f); // Tipo 5: Topo segue o Piso de sIdxBot (degrau)
                }
            }
            // Parede superior (rebaixamento de teto / porta)
            if (otherSector.ceilingHeight < sector.ceilingHeight) {
                std::string utex = NormalizeName(side.upperTexture);
                // Se a textura não for vazia
                if (utex != "-" && !utex.empty()) { 
                    auto tex = GetOrLoadWallTex(side.upperTexture, wad);
                    // Adiciona o quadrilátero de parede superior
                    addWall(v1, v2,
                            (float)otherSector.ceilingHeight, (float)sector.ceilingHeight,
                            otherSide.sector, side.sector,
                            sOff, xOff, yOff, light, tex,
                            4.0f, 2.0f); // Tipo 4: Base segue o Teto de sIdxBot (porta)
                }
            }
        }
    }

    // --- Pisos e Tetos via Ear Clipping ---
    for (int sIdx = 0; sIdx < (int)sectors.size(); ++sIdx) {
        // Pega os dados do setor
        const auto& sector = sectors[sIdx];
        float floor = (float)sector.floorHeight;
        float ceil  = (float)sector.ceilingHeight;
        float light = (float)sector.lightLevel / 255.0f;
        // Pega as texturas
        auto floorTex = GetOrLoadFlatTex(sector.floorTexture, wad);
        bool isSky    = NormalizeName(sector.ceilingTexture) == "F_SKY1";
        auto ceilTex  = isSky ? TexEntry{0,64,64} : GetOrLoadFlatTex(sector.ceilingTexture, wad);

        // Coleta as arestas pertencentes a este setor
        struct Edge { int v1, v2; };
        std::vector<Edge> edges;
        for (const auto& line : lineDefs) {
            if (line.rightSideDef != -1 && sideDefs[line.rightSideDef].sector == sIdx)
                edges.push_back({line.v1, line.v2});
            if (line.leftSideDef != -1 && sideDefs[line.leftSideDef].sector == sIdx)
                edges.push_back({line.v2, line.v1});
        }
        if (edges.empty()) continue;

        // 1. Agrupa as arestas em loops fechados
        std::vector<std::vector<int>> loops;
        while (!edges.empty()) {
            std::vector<int> loop;
            loop.push_back(edges[0].v1);
            int cur = edges[0].v2;
            edges.erase(edges.begin());
            bool found = true;
            while (found && cur != loop[0]) {
                found = false;
                for (size_t i = 0; i < edges.size(); ++i) {
                    if (edges[i].v1 == cur) {
                        loop.push_back(cur);
                        cur = edges[i].v2;
                        edges.erase(edges.begin() + i);
                        found = true;
                        break;
                    }
                }
            }
            if (loop.size() >= 3) loops.push_back(loop);
        }

        if (loops.empty()) continue;

        // 2. Preparação para o algoritmo Earcut
        using Point = std::array<double, 2>;
        std::vector<std::vector<Point>> polygon;

        // Auxiliar de área para distinguir contornos externos de buracos (holes)
        auto getArea = [&](const std::vector<int>& l) {
            double area = 0.0;
            for (size_t i = 0; i < l.size(); ++i) {
                const auto& v1 = mapVertices[l[i]];
                const auto& v2 = mapVertices[l[(i + 1) % l.size()]];
                area += (double)v1.x * v2.y - (double)v2.x * v1.y;
            }
            return area * 0.5;
        };

        // Ordena os loops para que aquele com a maior área absoluta (provavelmente o externo) venha primeiro
        // Em uma lógica mais robusta, verificaríamos se um loop está dentro de outro.
        // Para o mapa E1M1, o maior loop é quase sempre o externo.
        std::sort(loops.begin(), loops.end(), [&](const std::vector<int>& a, const std::vector<int>& b) {
            return std::abs(getArea(a)) > std::abs(getArea(b));
        });

        std::vector<int> allIndices; // Para mapear pontos indexados de volta aos vértices reais
        for (const auto& loop : loops) {
            std::vector<Point> ring;
            double area = getArea(loop);
            
            // Outer loop should be CCW (positive area in our coordinate system)
            // Holes should be CW (negative area)
            // Earcut funciona melhor quando as orientações são consistentes.
            bool isOuter = (polygon.empty());
            bool shouldBeCCW = isOuter; // Simplificado: o primeiro é o externo
            
            if ((area > 0) != shouldBeCCW) {
                // Inverte a ordem
                for (auto it = loop.rbegin(); it != loop.rend(); ++it) {
                    const auto& mv = mapVertices[*it];
                    ring.push_back({(double)mv.x, (double)mv.y});
                    allIndices.push_back(*it);
                }
            } else {
                for (int idx : loop) {
                    const auto& mv = mapVertices[idx];
                    ring.push_back({(double)mv.x, (double)mv.y});
                    allIndices.push_back(idx);
                }
            }
            polygon.push_back(ring);
        }

        // 3. Triangulação
        std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon);

        // 4. Geração de vértices
        for (size_t i = 0; i < indices.size(); i += 3) {
            int i0 = allIndices[indices[i]];
            int i1 = allIndices[indices[i+1]];
            int i2 = allIndices[indices[i+2]];

            const auto& mv0 = mapVertices[i0];
            const auto& mv1 = mapVertices[i1];
            const auto& mv2 = mapVertices[i2];

            float u0f = (float)mv0.x / 64.0f, v0f = (float)mv0.y / 64.0f;
            float u1f = (float)mv1.x / 64.0f, v1f = (float)mv1.y / 64.0f;
            float u2f = (float)mv2.x / 64.0f, v2f = (float)mv2.y / 64.0f;

            auto& fverts = batchMap[floorTex.id];
            float si = (float)sIdx;
            fverts.push_back({(float)mv0.x, floor, (float)mv0.y, u0f, v0f, light, light, light, si, 0.0f});
            fverts.push_back({(float)mv1.x, floor, (float)mv1.y, u1f, v1f, light, light, light, si, 0.0f});
            fverts.push_back({(float)mv2.x, floor, (float)mv2.y, u2f, v2f, light, light, light, si, 0.0f});

            if (!isSky) {
                auto& cverts = batchMap[ceilTex.id];
                // Inverte a ordem de enrolamento (winding) para o teto (1.0f indica teto)
                cverts.push_back({(float)mv0.x, ceil, (float)mv0.y, u0f, v0f, light, light, light, si, 1.0f});
                cverts.push_back({(float)mv2.x, ceil, (float)mv2.y, u2f, v2f, light, light, light, si, 1.0f});
                cverts.push_back({(float)mv1.x, ceil, (float)mv1.y, u1f, v1f, light, light, light, si, 1.0f});
            }
        }
    }

    // Sobe cada lote (batch) como seu próprio VAO/VBO
    for (auto& [texId, verts] : batchMap) {
        if (verts.empty()) continue;
        DrawBatch batch;
        batch.texId = texId;
        batch.count = (int)verts.size();

        glGenVertexArrays(1, &batch.vao);
        glGenBuffers(1, &batch.vbo);
        glBindVertexArray(batch.vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     verts.size() * sizeof(Vertex3D),
                     verts.data(), GL_STATIC_DRAW);

        // localização 0: posição (xyz)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
        glEnableVertexAttribArray(0);
        // localização 1: coordenadas de textura (uv)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // localização 2: cor da luz (rgb)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // location 3: sectorIndex
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
        // location 4: vertexType
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(4);
        // location 5: invTexHeight
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(10 * sizeof(float)));
        glEnableVertexAttribArray(5);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        mBatches.push_back(batch);
    }

    std::cout << "Scene: " << mBatches.size() << " draw batches, "
              << mTexCache.size() << " textures loaded." << std::endl;
}

void Scene::Render(const std::vector<float>& ceilOffsets, const std::vector<float>& floorOffsets, float time, const glm::vec3& camPos, const glm::vec3& camDir, bool flashlightOn) {
    // Configuração básica: assumindo que você tem uma forma de encontrar o ID do programa de shader
    GLint prog;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    
    // Atualiza a lanterna do jogador
    mRTManager.SetFlashlight(camPos, camDir, flashlightOn);
    // Sobe os deslocamentos (offsets) (até 512 setores)
    if (!ceilOffsets.empty())
        glUniform1fv(glGetUniformLocation(prog, "uSectorCeilOffsets"), (int)ceilOffsets.size(), ceilOffsets.data());
    if (!floorOffsets.empty())
        glUniform1fv(glGetUniformLocation(prog, "uSectorFloorOffsets"), (int)floorOffsets.size(), floorOffsets.data());

    glUniform1f(glGetUniformLocation(prog, "uTime"), time);

    // Associa (Bind) os buffers de RT
    mRTManager.Bind(prog);

    for (const auto& batch : mBatches) {
        glBindTexture(GL_TEXTURE_2D, batch.texId);
        glBindVertexArray(batch.vao);
        glDrawArrays(GL_TRIANGLES, 0, batch.count);
    }
    glBindVertexArray(0);
}
