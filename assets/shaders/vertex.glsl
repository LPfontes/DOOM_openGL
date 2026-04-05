#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aSectorIndex;
layout (location = 4) in float aVertexType; // 0:Floor, 1:Ceiling, 2:WallTop, 3:WallBottom
layout (location = 5) in float aInvTexHeight;

out vec2 TexCoord;
out vec3 LightColor;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float uSectorCeilOffsets[256];
uniform float uSectorFloorOffsets[256];

void main() {
    vec3 pos = aPos;
    int sIdx = int(aSectorIndex);
    vec2 uv = aTexCoord;
    float off = 0.0;

    // Type 1: Ceiling
    // Type 2: Wall Top (follows ceiling)
    // Type 4: Wall Bottom (follows ceiling - e.g. door face)
    if (aVertexType == 1.0 || aVertexType == 2.0 || aVertexType == 4.0) {
        off = uSectorCeilOffsets[sIdx];
        pos.y += off;
        // Scroll texture if it's a wall (Type 2 or 4)
        if (aVertexType > 1.5) uv.y -= off * aInvTexHeight;
    }
    
    // Type 0: Floor
    // Type 3: Wall Bottom (follows floor)
    // Type 5: Wall Top (follows floor - e.g. step face)
    if (aVertexType == 0.0 || aVertexType == 3.0 || aVertexType == 5.0) {
        off = uSectorFloorOffsets[sIdx];
        pos.y += off;
        // Scroll texture if it's a wall (Type 3 or 5)
        if (aVertexType > 1.5) uv.y -= off * aInvTexHeight;
    }

    WorldPos = (model * vec4(pos, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoord   = uv;
    LightColor = aColor;
}
