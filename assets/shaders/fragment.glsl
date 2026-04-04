#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 LightColor;

uniform sampler2D ourTexture;

void main() {
    FragColor = texture(ourTexture, TexCoord) * vec4(LightColor, 1.0);
}
