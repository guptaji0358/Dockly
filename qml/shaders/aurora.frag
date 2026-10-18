#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    float aspect;
    vec4 base;
    vec4 colorA;
    vec4 colorB;
    float intensity;
};

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise(p);
        p *= 2.03;
        a *= 0.5;
    }
    return v;
}

void main()
{
    vec2 uv = qt_TexCoord0;
    vec2 p = vec2(uv.x * aspect, uv.y);
    float t = time * 0.045;

    float n1 = fbm(p * 1.2 + vec2(t, -t * 0.6));
    float n2 = fbm(p * 2.0 - vec2(t * 0.8, t) + n1 * 1.6);
    float ribbon = smoothstep(0.38, 0.85, n2) * smoothstep(1.05, 0.25, uv.y + n1 * 0.35);

    vec3 col = base.rgb;
    col += colorA.rgb * (n1 * n1) * 0.9 * intensity;
    col += colorB.rgb * ribbon * 0.75 * intensity;

    float vig = smoothstep(1.25, 0.25, length(uv - 0.5) * 1.5);
    col *= mix(0.5, 1.0, vig);
    fragColor = vec4(col, 1.0) * qt_Opacity;
}
