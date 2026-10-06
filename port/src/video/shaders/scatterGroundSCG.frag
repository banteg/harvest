// GLSL ES 3.00 / GLSL 3.30 translation of harvestClientData/gfx/shaders/scatterGroundSCG.psh: the
// low shader level's planet lighting (diffuse with 0.1 ambient, specular and glow, no scattering).
// Pixel-program uniforms carry the prefix ps_; DiffSpec is texture unit 0 (TEX0), NormGlow unit 1
// (TEX1). The #version line is prepended at run time.

uniform sampler2D ps_DiffSpec;
uniform sampler2D ps_NormGlow;

uniform vec3 ps_v3LightPos;
uniform vec3 ps_v3CameraPos;

// TEXCOORD0, TEXCOORD1
in vec2 vTexCoord0;
in vec4 vNormal;

out vec4 fragColor;

void main()
{
    // Extract diffuse and specular components from texture
    vec4 fDiffSpecMap = texture(ps_DiffSpec, vTexCoord0);
    vec4 fBaseColor = vec4(fDiffSpecMap.rgb, 1.0);

    // Extract normals and glow from texture
    vec4 fNormGlow = texture(ps_NormGlow, vTexCoord0);

    vec3 vLightDirection = normalize(ps_v3LightPos);
    vec3 vNorm = vNormal.xyz;

    // Some calculations use the regular surface normal and some use the bumped
    float fNDotL = dot(vNorm, vLightDirection);

    vec3 vReflection = normalize(((2.0 * vNorm) * (fNDotL)) - vLightDirection);

    vec3 vViewDirection = normalize(ps_v3CameraPos);
    float fRDotV = max(0.0, dot(vReflection, vViewDirection));

    // Calculate specular component
    // Shininess is hard-coded, but a scaling is applied from the texture map
    float spec = pow(fRDotV, 25.0);
    vec4 fTotalSpecular = spec * fDiffSpecMap.a * fBaseColor;

    // Diffuse component, used together with scattering
    float fDiffuseC = max(0.0, fNDotL);
    vec4 fTotalDiffuse = (fDiffuseC + 0.1) * fBaseColor;

    // Calculate glow. The pow is there to provide faster fallof in the ramp area. (The max only
    // guards against rounding below 0, where pow is undefined.)
    vec4 fTotalGlow = fNormGlow.a * 1.7 * pow(max(1.0 - fDiffuseC, 0.0), 5.0) * fBaseColor;

    // The whole is not greater than the sum of all components
    fragColor = clamp(fTotalSpecular + fTotalDiffuse + fTotalGlow, 0.0, 1.0);
}
