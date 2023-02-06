#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform sampler2DArray paletteTexture;

void main()
{
    vec2 plt_data = texture(ourTexture, TexCoord).xy;
    FragColor = texture(paletteTexture, vec3(1, 1, 1));
}
