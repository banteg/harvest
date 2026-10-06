// GLSL ES 3.00 / GLSL 3.30 translation of harvestClientData/gfx/shaders/scatterAtmoCG.psh: the
// Rayleigh and Mie phase functions of Sean O'Neil's sky from space (copyright (c) 2004 Sean
// O'Neil). Pixel-program uniforms carry the prefix ps_. The #version line is prepended at run time.

uniform vec3 ps_v3LightPos;

// COLOR0, COLOR1, TEXCOORD0
in vec3 vFrontColor;
in vec3 vSecondaryColor;
in vec3 vDirection;

out vec4 fragColor;

void main()
{
    float g = -0.95;
    float g2 = 0.9025;

    float fCos = dot(ps_v3LightPos, vDirection) / length(vDirection);
    float fRayleighPhase = 0.75 * (1.0 + fCos * fCos);
    float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos * fCos) / pow(1.0 + g2 - 2.0 * g * fCos, 1.5);

    fragColor.rgb = (fRayleighPhase * vFrontColor + fMiePhase * vSecondaryColor);
    fragColor.a = 1.0;
}
