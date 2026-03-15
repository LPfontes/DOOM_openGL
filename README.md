# DOOM OpenGL Engine 🚀🎮

Um motor de renderização 3D simplificado construído em C++ e OpenGL para visualizar mapas originais do Doom (.WAD).

## 🌟 Funcionalidades

- **Parser de WAD**: Leitura completa de cabeçalho, diretório e lumps de arquivos WAD.
- **Renderização 3D de Geometria**:
  - **Paredes**: Geração de geometria para linhas sólidas (1-sided) e portais (2-sided), tratando diferenças de altura entre setores (degraus, janelas).
  - **Pisos e Tetos**: Implementação robusta usando o algoritmo de **Ear Clipping** para triangulação de setores côncavos, garantindo uma cena sem buracos.
  - **Uso de SEGS**: Renderização baseada em segmentos pré-calculados para maior fidelidade ao design original.
- **Navegação 3D**:
  - Controle de câmera estilo FPS (WASD + Mouse).
  - Sistema de "voo" para exploração técnica do mapa.
  - Posicionamento automático no **Player 1 Start** original.
- **Sistema de Cores por Altura**: Visualização facilitada por cores (Azul/Verde/Vermelho) para distinguir diferentes níveis de elevação do mapa.
- **Iluminação por Setor**: Suporte aos níveis de brilho original do motor Doom para profundidade visual.

## 🛠️ Tecnologias Utilizadas

- **C++**: Lógica principal e processamento de dados binários.
- **OpenGL 3.3 (Core Profile)**: Pipeline de renderização moderno.
- **GLFW**: Gerenciamento de janelas e entrada.
- **GLAD**: Carregamento de extensões OpenGL.
- **GLM (OpenGL Mathematics)**: Operações de matrizes e vetores 3D.
- **CMake**: Sistema de build multiplataforma.

## 📁 Estrutura do Projeto

- `src/main.cpp`: Orquestrador da aplicação, loop de renderização e input.
- `src/WADParser.cpp`: Lógica de leitura de arquivos binários WAD.
- `src/Map.cpp`: Extração e organização dos dados de mapas (Vertices, LineDefs, Sectors, Things).
- `src/Scene.cpp`: O "coração" geométrico; converte dados 2D do Doom em triângulos 3D (Ear Clipping).
- `include/Camera.h`: Classe de câmera Euler para navegação.
- `assets/shaders/`: Shaders GLSL para processamento de vértices e fragmentos.

## 🚀 Como Executar

### Pré-requisitos
- Compilador C++ (recomendado MinGW no Windows ou GCC no Linux).
- CMake instalado.
- Arquivo `doom1.wad` (Shareware ou Full) na pasta `assets/`.

### Comandos de Build (Windows/MinGW)
```powershell
# Configurar o projeto
cmake -G "MinGW Makefiles" -B build -S .

# Compilar
cmake --build build

# Executar
.\build\DOOM_OpenGL.exe
```

## 🎮 Controles
- **W, A, S, D**: Movimentação.
- **Mouse**: Olhar ao redor.
- **Q / E**: Subir / Descer.
- **ESC**: Fechar aplicação.

---
*Projeto desenvolvido como parte de um estudo sobre engines de jogos clássicos e computação gráfica.*
