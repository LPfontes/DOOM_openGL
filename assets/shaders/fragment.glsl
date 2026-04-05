#version 430 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 LightColor;
in vec3 WorldPos;

struct GPULight {
    vec4 posType;   // xyz, type (0: steady, 1: flicker, 2: pulse, 3: spotlight)
    vec4 colorInt;  // rgb, intensity
    vec4 dirCutoff; // dx, dy, dz, cutoff
    float radius;
};

struct GPULine {
    vec2 p1;
    vec2 p2;
    float floor;
    float ceil;
    int sectorIdx;
};

layout (std430, binding = 1) buffer LightBuffer {
    GPULight lights[];
};
layout (std430, binding = 2) buffer LineBuffer {
    GPULine lines[];
};

uniform int uNumLights;
uniform int uNumLines;
uniform float uTime;
uniform sampler2D ourTexture;
uniform float uSectorCeilOffsets[256];
uniform float uSectorFloorOffsets[256];

// Ray-segment intersection
bool IsShadowed(vec3 fPos, vec3 lPos) {
    vec2 rO = fPos.xz;
    vec2 rDir = lPos.xz - fPos.xz;
    float dMax = length(rDir);
    if (dMax < 0.001) return false;
    vec2 rD = rDir / dMax;

    for (int i = 0; i < uNumLines; ++i) {
        vec2 p1 = lines[i].p1 * 0.01; // Scaled to world units
        vec2 p2 = lines[i].p2 * 0.01;
        
        vec2 v1 = rO - p1;
        vec2 v2 = p2 - p1;
        vec2 v3 = vec2(-rD.y, rD.x);

        float det = dot(v2, v3);
        if (abs(det) < 0.00001) continue;

        float t = (v2.x * v1.y - v2.y * v1.x) / det;
        float u = dot(v1, v3) / det;

        if (t > 0.01 && t < dMax - 0.01 && u >= 0.0 && u <= 1.0) {
            // Check vertical blocking with offsets
            float h = fPos.y + (lPos.y - fPos.y) * (t / dMax);
            int sIdx = lines[i].sectorIdx;
            
            float f = lines[i].floor * 0.01;
            float c = lines[i].ceil * 0.01;

            if (sIdx >= 0 && sIdx < 256) {
                f += uSectorFloorOffsets[sIdx];
                c += uSectorCeilOffsets[sIdx];
            }
            
            if (h < f || h > c) return true;
        }
    }
    return false;
}

float GetFlicker(int type, float seed) {
    if (type == 1) { // Flicker
        return 0.8 + 0.2 * sin(uTime * 15.0 + seed * 10.0) * cos(uTime * 7.0 + seed * 5.0);
    }
    if (type == 2) { // Pulse
        return 0.7 + 0.3 * sin(uTime * 2.0 + seed);
    }
    return 1.0;
}

void main() {
    vec4 texColor = texture(ourTexture, TexCoord);
    if (texColor.a < 0.1) discard;

    // Minimum ambient light based on sector light
    vec3 ambient = texColor.rgb * LightColor * 0.15;
    vec3 totalDiffuse = vec3(0.0);

    for (int i = 0; i < uNumLights; ++i) {
        vec3 lPos = lights[i].posType.xyz * 0.01; 
        float type = lights[i].posType.w;
        vec3 lCol = lights[i].colorInt.rgb;
        float lInt = lights[i].colorInt.a;
        float rad = lights[i].radius * 0.01;

        vec3 toLight = lPos - WorldPos;
        float d = length(toLight);
        
        if (d < rad * 5.0) {
            float atten = lInt / (1.0 + (2.0/rad)*d + (1.0/(rad*rad))*d*d);
            
            if (type == 3.0) { // Spotlight
                vec3 L = normalize(-toLight); // Direction from light TO fragment
                vec3 D = normalize(lights[i].dirCutoff.xyz);
                float theta = dot(L, D);
                float cutoff = lights[i].dirCutoff.w;
                if (theta > cutoff) {
                    float epsilon = 0.05;
                    atten *= clamp((theta - cutoff) / epsilon, 0.0, 1.0);
                } else {
                    atten = 0.0;
                }
            } else {
                atten *= GetFlicker(int(type), float(i));
            }

            if (atten > 0.01) {
                if (!IsShadowed(WorldPos, lPos)) {
                    totalDiffuse += lCol * atten;
                }
            }
        }
    }

    vec3 finalColor = ambient + texColor.rgb * totalDiffuse;
    FragColor = vec4(finalColor, texColor.a);
}
