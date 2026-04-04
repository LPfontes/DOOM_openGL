# DOOM OpenGL Engine 🚀🎮

Um motor de renderização 3D simplificado construído em C++ e OpenGL para visualizar mapas originais do Doom (.WAD). Recentemente atualizado para incluir um sistema de movimentação física realista e triangulação avançada.

## 🌟 Novas Funcionalidades

- **Física de Movimentação FPS**:
  - **Colisão com Deslizamento**: Sistema de colisão círculo-contra-parede com projeção vetorial, permitindo deslizar suavemente por paredes diagonais.
  - **Subida de Escadas Suave**: Implementação de *Step Climbing* com interpolação linear (**Lerp**) para transições de altura fluidas.
  - **Modo Corrida**: Pressione `Shift` para aumentar a velocidade de deslocamento.
- **Detecção de Setores via BSP**: Uso da árvore BSP original do WAD para identificar instantaneamente o setor e as elevações (chão/teto) sob os pés do jogador.
- **Triangulação Robusta com Earcut**: Integração da biblioteca `earcut.hpp` para processar setores complexos com buracos e ilhas, eliminando artefatos visuais no chão e teto.

## 🌟 Funcionalidades Base

- **Parser de WAD**: Leitura completa de cabeçalho, diretório e lumps de arquivos WAD.
- **Renderização 3D de Geometria**:
  - **Paredes**: Geração de geometria para linhas sólidas (1-sided) e portais (2-sided), tratando diferenças de altura entre setores.
  - **Uso de SEGS**: Renderização baseada em segmentos pré-calculados para maior fidelidade ao design original.
- **Texturas Reais do Doom**: Carregamento e renderização das texturas originais diretamente do arquivo WAD.
  - **Flats e Wall Textures**: Decodificação completa de patches, pnames e texturas compostas via paleta `PLAYPAL`.
  - **Batch Rendering**: Malhas agrupadas por textura para alta performance.
- **Iluminação por Setor**: O `lightLevel` original modula a luz via fragment shader.

## 🛠️ Tecnologias Utilizadas

- **C++17**: Lógica principal e processamento binário.
- **OpenGL 3.3 (Core Profile)**: Pipeline de renderização moderno.
- **GLFW / GLAD**: Janelas e extensões.
- **GLM**: Matemática 3D.
- **Earcut.hpp**: Triangulação robusta de polígonos.
- **CMake**: Sistema de build.

## 📁 Estrutura do Projeto

- `src/Movement.cpp`: Lógica de física, colisão e movimentação do jogador.
- `src/Map.cpp`: Processamento da árvore BSP e dados do WAD.
- `src/Scene.cpp`: Geração de malhas 3D e triangulação avançada.
- `src/WADParser.cpp`: Parser de baixo nível para o formato WAD.

## 🎮 Controles

- **W, A, S, D**: Movimentação.
- **Shift (Segurar)**: Correr.
- **Mouse**: Olhar ao redor (pitch/yaw limitado).
- **Q / E**: Subir / Descer (Modo Debug/Voo).
- **ESC**: Sair.

## 🚀 Como Compilar e Executar

Consulte as instruções detalhadas de build para Windows (MinGW) ou Linux no final deste arquivo. Certifique-se de ter o `doom1.wad` na pasta `assets/`.

---
*Projeto desenvolvido para estudo de motores de jogos clássicos e computação gráfica.*
