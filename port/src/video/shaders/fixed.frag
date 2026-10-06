// OpenGL 1.x's fixed-function fragment stage as the original renderer used it: two texture units
// with their GL_TEXTURE_2D enables and texture environments (MODULATE, DECAL, REPLACE, ADD and the
// light-map COMBINE), the alpha test (GL_GREATER), and linear or exponential fog. The #version
// line is prepended at run time.

in vec4 vColor;
in vec2 vTexCoord0;
in vec2 vTexCoord1;
in float vFogCoord;

uniform sampler2D uTexture0;
uniform sampler2D uTexture1;
uniform bool uTextureEnabled[2];
// 0 MODULATE, 1 DECAL, 2 REPLACE, 3 ADD, 4 COMBINE
uniform int uTexEnvMode[2];
// GL_COMBINE_RGB: 0 REPLACE, 1 MODULATE
uniform int uCombineRgb[2];
// GL_SOURCE0_RGB, GL_SOURCE1_RGB: 0 TEXTURE, 1 PREVIOUS, 2 CONSTANT (black)
uniform int uCombineSource0[2];
uniform int uCombineSource1[2];
uniform float uRgbScale[2];

uniform bool uAlphaTest;
uniform float uAlphaRef;

// 0 off, 1 linear, 2 exponential
uniform int uFogMode;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uFogDensity;
uniform vec4 uFogColor;

out vec4 fragColor;

vec3 combineSource(int source, vec4 previous, vec4 texel)
{
    if (source == 0)
        return texel.rgb;
    if (source == 1)
        return previous.rgb;
    return vec3(0.0);
}

vec4 textureEnvironment(int mode, int combineRgb, int source0, int source1, float rgbScale, vec4 previous,
    vec4 texel)
{
    vec4 result;
    if (mode == 0)
        result = previous * texel;
    else if (mode == 1)
        result = vec4(mix(previous.rgb, texel.rgb, texel.a), previous.a);
    else if (mode == 2)
        result = texel;
    else if (mode == 3)
        result = vec4(previous.rgb + texel.rgb, previous.a * texel.a);
    else
    {
        vec3 arg0 = combineSource(source0, previous, texel);
        vec3 arg1 = combineSource(source1, previous, texel);
        vec3 rgb = combineRgb == 0 ? arg0 : arg0 * arg1;
        result = vec4(rgb * rgbScale, previous.a * texel.a);
    }
    return clamp(result, 0.0, 1.0);
}

void main()
{
    vec4 color = vColor;

    if (uTextureEnabled[0])
        color = textureEnvironment(uTexEnvMode[0], uCombineRgb[0], uCombineSource0[0], uCombineSource1[0],
            uRgbScale[0], color, texture(uTexture0, vTexCoord0));
    if (uTextureEnabled[1])
        color = textureEnvironment(uTexEnvMode[1], uCombineRgb[1], uCombineSource0[1], uCombineSource1[1],
            uRgbScale[1], color, texture(uTexture1, vTexCoord1));

    if (uFogMode != 0)
    {
        float f = uFogMode == 1 ? (uFogEnd - vFogCoord) / (uFogEnd - uFogStart) : exp(-uFogDensity * vFogCoord);
        color.rgb = mix(uFogColor.rgb, color.rgb, clamp(f, 0.0, 1.0));
    }

    if (uAlphaTest && !(color.a > uAlphaRef))
        discard;

    fragColor = color;
}
