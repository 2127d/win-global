#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 surfaceSize;
    float cornerRadius;
    float refraction;
    float bevelWidth;
};
layout(binding = 1) uniform sampler2D source;

float distanceToGlass(vec2 p) {
    vec2 q = abs(p) - surfaceSize * 0.5 + cornerRadius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cornerRadius;
}
void main() {
    vec2 p = (qt_TexCoord0 - 0.5) * surfaceSize;
    float depth = max(-distanceToGlass(p), 0.0);
    vec2 gradient = vec2(distanceToGlass(p + vec2(0.5,0)) - distanceToGlass(p - vec2(0.5,0)),
                         distanceToGlass(p + vec2(0,0.5)) - distanceToGlass(p - vec2(0,0.5)));
    vec2 outward = gradient / max(length(gradient), 0.0001);
    float rim = 1.0 - smoothstep(0.0, max(bevelWidth,0.01), depth);
    float bevel = 0.88 * rim;
    vec3 normal = vec3(outward * bevel, sqrt(1.0 - bevel * bevel));
    vec3 ray = refract(vec3(0,0,-1), normal, 1.0 / 1.45);
    vec2 displacement = ray.xy / max(-ray.z,0.1) * refraction * 2.0;
    displacement -= p * 0.09 * (0.30 + 0.70 * clamp(dot(p / (surfaceSize * 0.5),p / (surfaceSize * 0.5))*0.5,0.0,1.0));
    displacement *= min(1.0,64.0 / max(length(displacement),0.001));
    vec2 uv = clamp(qt_TexCoord0 + displacement / surfaceSize, vec2(0.001), vec2(0.999));
    vec4 color = texture(source,uv);
    vec3 light = normalize(vec3(-0.455,-0.455,0.766));
    float key = max(dot(outward,normalize(light.xy)),0.0);
    float opposite = max(dot(outward,-normalize(light.xy)),0.0);
    float specular = pow(max(dot(normal,normalize(light+vec3(0,0,1))),0.0),48.0)*bevel;
    float edge = exp(-depth/1.7);
    float fresnel = 0.04 + 0.96*pow(1.0-normal.z,5.0);
    color.rgb *= 1.0 - 0.34*opposite*bevel*0.23;
    color.rgb = mix(color.rgb,vec3(1),clamp(0.34*(specular*2.0+edge*(0.25+key)+fresnel*bevel),0.0,0.65));
    fragColor = color * qt_Opacity;
}
