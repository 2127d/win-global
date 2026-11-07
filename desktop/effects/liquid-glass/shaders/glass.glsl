// SPDX-License-Identifier: GPL-2.0-or-later
// Applied to the composited backdrop BEFORE the application paints its content.
uniform sampler2D rawBackdrop;
uniform vec4 glassFrame;
uniform vec4 glassRadius;
uniform vec2 glassTextureSize;
uniform float glassRefraction;
uniform float glassRim;
uniform float glassClarityRim;
uniform float glassScale;
uniform float glassLensStrength;
uniform float glassEdgeClarity;
uniform float glassCenterClarity;
uniform float glassEdgeThickness;
uniform vec3 glassLight;
uniform float glassLightStrength;

float glassDistance(vec2 p) {
    return sdfRoundedBox(p, glassFrame.xy, glassFrame.zw, glassRadius);
}

vec4 glassBackdrop(vec2 coordinate, vec2 point) {
    float distance = glassDistance(point);
    float depth = max(-distance, 0.0);
    float epsilon = max(glassScale * 0.5, 0.5);
    vec2 gradient = vec2(glassDistance(point + vec2(epsilon, 0.0)) - glassDistance(point - vec2(epsilon, 0.0)),
                         glassDistance(point + vec2(0.0, epsilon)) - glassDistance(point - vec2(0.0, epsilon)));
    if (max(max(glassRadius.x, glassRadius.y), max(glassRadius.z, glassRadius.w)) < glassScale) {
        vec2 localNormal = point - glassFrame.xy;
        vec2 sideDistance = abs(localNormal) - glassFrame.zw;
        float blend = smoothstep(-2.0 * glassScale, 2.0 * glassScale, sideDistance.x - sideDistance.y);
        gradient = sign(localNormal) * vec2(blend, 1.0 - blend);
    }
    vec2 outward = gradient / max(length(gradient), 0.0001);
    float rim = 1.0 - smoothstep(0.0, glassRim, depth);
    float bevel = 0.88 * rim;
    vec3 normal = vec3(outward * bevel, sqrt(1.0 - bevel * bevel));
    // Snell's law, air -> glass (IOR 1.45). Fixed screen-space light, no shimmer.
    vec3 ray = refract(vec3(0.0, 0.0, -1.0), normal, 1.0 / 1.45);
    vec2 displacement = ray.xy / max(-ray.z, 0.1) * glassRefraction * 2.0;
    // A shallow aspheric lens bends the broad interior as well as the bevel.
    // Move samples toward the center; the capped displacement also bounds damage.
    vec2 local = point - glassFrame.xy;
    vec2 normalized = local / max(glassFrame.zw, vec2(1.0));
    float radial = clamp(dot(normalized, normalized) * 0.5, 0.0, 1.0);
    displacement -= local * glassLensStrength * (0.30 + 0.70 * radial);
    float travel = length(displacement);
    displacement *= min(1.0, 64.0 * glassScale / max(travel, 0.001));
    vec2 sampleUV = coordinate + displacement * vec2(1.0, -1.0) / glassTextureSize;
    // Every tap stays inside the captured backdrop; no edge-wrapped pixels.
    vec2 guard = halfpixel * (2.0 * offset + 1.0);
    sampleUV = clamp(sampleUV, guard, vec2(1.0) - guard);
    vec4 sum = texture(texUnit, sampleUV + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, sampleUV + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, sampleUV + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, sampleUV + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, sampleUV + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, sampleUV + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, sampleUV + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, sampleUV + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;
    // Preserve visible features in the lens rim instead of blurring away the bend.
    // Clarity is independent of geometry, so zero-warp comparison isolates distortion.
    float clarityRim = 1.0 - smoothstep(0.0, glassClarityRim, depth);
    float clarity = mix(glassCenterClarity, glassEdgeClarity, pow(clarityRim, 0.7));
    vec4 color = mix(sum / 12.0, texture(rawBackdrop, sampleUV), clarity) * colorMatrix;
    vec3 halfVector = normalize(normalize(glassLight) + vec3(0.0, 0.0, 1.0));
    float specular = pow(max(dot(normal, halfVector), 0.0), 48.0) * bevel;
    float fresnel = 0.04 + 0.96 * pow(1.0 - normal.z, 5.0);
    float edge = exp(-depth / (1.7 * glassScale));
    float keyLight = max(dot(outward, normalize(glassLight.xy)), 0.0);
    float opposite = max(dot(outward, -normalize(glassLight.xy)), 0.0);
    float highlight = glassLightStrength * (specular * 2.0 + edge * (0.25 + keyLight) + fresnel * bevel);
    color.rgb *= 1.0 - glassLightStrength * opposite * bevel * 0.23;
    color.rgb = mix(color.rgb, vec3(1.0), clamp(highlight, 0.0, 0.65));
    // A polished outer lip, broad shoulder and inset shade describe solid glass
    // thickness. The normal follows the rounded perimeter and the scene light.
    float thickness = max(glassEdgeThickness, glassScale);
    float enabled = smoothstep(0.0, glassScale, glassEdgeThickness);
    float lip = 1.0 - smoothstep(0.65 * glassScale, 1.85 * glassScale, depth);
    float shoulder = exp(-pow((depth - thickness * 0.36) / (thickness * 0.42), 2.0));
    float innerShade = exp(-pow((depth - thickness) / (thickness * 0.30 + glassScale), 2.0));
    float polish = enabled * (0.55 + glassLightStrength)
        * (lip * (0.18 + 0.48 * keyLight) + shoulder * (0.065 + 0.15 * keyLight));
    color.rgb *= 1.0 - enabled * innerShade * (0.10 + 0.15 * opposite);
    color.rgb = mix(color.rgb, vec3(1.0), clamp(polish, 0.0, 0.80));
    return color;
}
