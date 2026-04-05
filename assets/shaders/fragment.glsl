#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 LightColor;
in vec3 WorldPos;  // Novo input do vertex

uniform vec3 lightPos;  
uniform sampler2D ourTexture;

void main() {
    vec4 texColor = texture(ourTexture, TexCoord);
    vec3 finalColor = texColor.rgb * LightColor;

    // Calcula iluminação dinâmica
    float dist = length(lightPos - WorldPos);
    float attenuation = 1.0 / (1.0 + 0.3 * dist + 0.01 * dist * dist); // Ajuste de luminosidade no cenario aqui
    finalColor *= attenuation;  // Aplica atenuação

    FragColor = vec4(finalColor, texColor.a);
}
