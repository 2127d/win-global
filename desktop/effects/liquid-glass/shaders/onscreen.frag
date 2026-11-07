#version 140
#include "sdf.glsl"

uniform sampler2D texUnit;
uniform mat4 colorMatrix;
uniform float offset;
uniform vec2 halfpixel;

in vec2 uv;
in vec2 vertex;

out vec4 fragColor;

#include "sequoia_liquid_glass_v6.glsl"

void main(void)
{
    fragColor = glassBackdrop(uv, vertex);
}
