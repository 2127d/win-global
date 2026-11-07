#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 surfaceSize;
    float cornerRadius;
    float rimIntensity;
    float backdropAvailable;
};
layout(binding = 1) uniform sampler2D source;
void main() {
    float radius = clamp(cornerRadius,0.0,min(surfaceSize.x,surfaceSize.y)*.5);
    vec2 p = (qt_TexCoord0-.5)*surfaceSize;
    vec2 q = abs(p)-surfaceSize*.5+radius;
    vec2 curved = max(q,0.0);
    float distance = min(max(q.x,q.y),0.0)+length(curved)-radius;
    float aa = max(fwidth(distance),.65);
    float coverage = 1.0-smoothstep(-aa,0.0,distance);
    float depth = max(-distance,0.0);
    // Reflection color comes from the transmitted scene, never a hue palette.
    float corner = smoothstep(.02,.34,curved.x*curved.y/max(radius*radius,1.0));
    float edge = exp(-pow((depth-1.3)/1.25,2.0));
    float inner = .20*exp(-pow((depth-3.7)/2.1,2.0));
    vec3 reflection = vec3(.92);
    if (backdropAvailable > .5) {
        vec4 scene = texture(source,clamp(qt_TexCoord0,vec2(.001),vec2(.999)));
        vec3 sceneColor = scene.rgb/max(scene.a,.001);
        reflection = mix(sceneColor,vec3(1.0),.22);
    }
    // Flat mode sees the desktop through alpha; do not sample hidden scenery
    // or pretend that the app has access to the compositor's backdrop texture.
    float alpha = (edge+inner)*(.10+.40*corner)*clamp(rimIntensity,0.0,2.0)*coverage*qt_Opacity;
    alpha = min(alpha,.75*qt_Opacity);
    fragColor = vec4(reflection*alpha,alpha);
}
