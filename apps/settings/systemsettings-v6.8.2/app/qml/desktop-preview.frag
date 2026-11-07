// SPDX-License-Identifier: GPL-2.0-or-later
#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 surfaceSize;
    vec2 sceneSize;
    vec4 sceneRect;
    float cornerRadius;
    float refraction;
};
layout(binding = 1) uniform sampler2D source;
vec4 scene(vec2 uv) { return texture(source, clamp(uv, vec2(.001), vec2(.999))); }
void main() {
    vec2 p = (qt_TexCoord0 - .5) * surfaceSize;
    float radius = min(cornerRadius, min(surfaceSize.x, surfaceSize.y) * .5);
    vec2 q = abs(p) - surfaceSize * .5 + radius;
    vec2 curved = max(q, 0.0);
    float distance = min(max(q.x, q.y), 0.0) + length(curved) - radius;
    float coverage = 1.0 - smoothstep(-max(fwidth(distance), .65), 0.0, distance);
    vec2 direction = length(curved) > .001 ? normalize(curved) * sign(p)
        : (q.x > q.y ? vec2(sign(p.x), 0.0) : vec2(0.0, sign(p.y)));
    float depth = max(-distance, 0.0);
    float bend = exp(-pow(depth / max(radius * .65, 2.0), 2.0));
    vec2 uv = sceneRect.xy + qt_TexCoord0 * sceneRect.zw;
    uv -= direction * bend * (radius * .5) * refraction / sceneSize;
    // The center stays clear; only the rounded edge refracts. Transparent
    // previews use broader frosting, matching the desktop material recipe.
    vec2 blur = vec2(mix(2.8, .45, refraction)) / sceneSize;
    vec4 color = scene(uv) * .40;
    color += (scene(uv + vec2(blur.x, 0)) + scene(uv - vec2(blur.x, 0))
           + scene(uv + vec2(0, blur.y)) + scene(uv - vec2(0, blur.y))) * .15;
    fragColor = color * coverage * qt_Opacity;
}
