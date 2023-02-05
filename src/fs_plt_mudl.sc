$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor,  0);
SAMPLER2DARRAY(s_paletteColor,  1);

void main()
{
    vec2 plt = texture2D(s_texColor, v_texcoord0).xy;
    gl_FragColor = bgfxTexture2DArray(s_paletteColor, vec3(0, 20, 0));
}
