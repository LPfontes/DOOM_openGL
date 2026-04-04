#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aSectorIndex;
layout (location = 4) in float aVertexType; // 0:Floor, 1:Ceil, 2:WallTop, 3:WallBottom

out vec2 TexCoord;
out vec3 LightColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float uSectorCeilOffsets[256];
uniform float uSectorFloorOffsets[256];

void main() {
    vec3 pos = aPos;
    int sIdx = int(aSectorIndex);
    
    // Apply offsets based on vertex type
    // 1: Ceiling, 2: Wall Top (follows ceiling)
    if (aVertexType > 0.5 && aVertexType < 2.5) {
        pos.y += uSectorCeilOffsets[sIdx];
    }
    // 0: Floor, 3: Wall Bottom (follows floor)
    if (aVertexType < 0.5 || aVertexType > 2.5) {
        pos.y += uSectorFloorOffsets[sIdx];
    }

    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoord   = aTexCoord;
    LightColor = aColor;
}
