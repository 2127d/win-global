#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 surfaceSize;
    float cornerRadius;
    float refractionStrength;
    float bevelWidth;
    float dispersion;
};
layout(binding = 1) uniform sampler2D source;
float distanceToSurface(vec2 p) {
    float radius = clamp(cornerRadius, 0.0, min(surfaceSize.x, surfaceSize.y) * 0.5);
    vec2 q = abs(p) - surfaceSize * 0.5 + radius;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - radius;
}
void main() {
    vec2 p = (qt_TexCoord0 - 0.5) * surfaceSize;
    float distance = distanceToSurface(p);
    float depth = max(-distance, 0.0);
    vec2 gradient = vec2(distanceToSurface(p+vec2(.5,0))-distanceToSurface(p-vec2(.5,0)),
                         distanceToSurface(p+vec2(0,.5))-distanceToSurface(p-vec2(0,.5)));
    vec2 outward = gradient / max(length(gradient), .0001);
    // End the bevel before the rounded rectangle's medial seams.
    float safeBevel = max(0.5, min(bevelWidth, min(cornerRadius, min(surfaceSize.x, surfaceSize.y)*0.5)*0.8));
    float bevel = .82 * (1.0-smoothstep(0.0,safeBevel,depth));
    vec3 normal = vec3(outward*bevel,sqrt(1.0-bevel*bevel));
    vec3 ray = refract(vec3(0,0,-1), normal, 1.0/1.45);
    vec2 displacement = ray.xy / max(-ray.z,.1)*refractionStrength;
    vec2 uv = clamp(qt_TexCoord0 + displacement / surfaceSize, vec2(.001), vec2(.999));
    float coverage = 1.0-smoothstep(-max(fwidth(distance),.5),0.0,distance);
    vec4 sampleColor = texture(source,uv);
    if (dispersion > 0.0) {
        // Dispersion stays in the bevel, strongest around the curved corners.
        // Never split foreground text: this texture contains only scenery.
        float corner = smoothstep(.08, .45, abs(outward.x * outward.y));
        vec2 split = outward * bevel * dispersion * (.35 + .9 * corner) / surfaceSize;
        sampleColor.r = texture(source,clamp(uv+split,vec2(.001),vec2(.999))).r;
        sampleColor.b = texture(source,clamp(uv-split,vec2(.001),vec2(.999))).b;
        sampleColor.rgb = min(sampleColor.rgb,vec3(sampleColor.a));
    }
    fragColor = sampleColor*coverage*qt_Opacity;
}
