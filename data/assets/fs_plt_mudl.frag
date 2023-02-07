#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform sampler2D paletteTextures[10];

void main()
{
    vec2 plt_data = texture(ourTexture, TexCoord).xy;
    int actual_layer = int(max(0, min(9, floor(plt_data.y + 0.5))));
    int actual_color = int(max(0, min(255, floor(plt_data.x + 0.5))));
    FragColor = texture(paletteTextures[actual_layer], vec2(plt_data.x, 0));
}
