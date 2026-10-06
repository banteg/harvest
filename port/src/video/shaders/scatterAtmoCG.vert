// GLSL ES 3.00 / GLSL 3.30 translation of harvestClientData/gfx/shaders/scatterAtmoCG.vsh:
// Sean O'Neil's atmospheric scattering, sky from space (GPU Gems 2, chapter 16; copyright (c) 2004
// Sean O'Neil). Vertex-program uniforms carry the prefix vs_ so that they keep their own namespace,
// as Cg's separate programs did. The #version line is prepended at run time.

uniform mat4 vs_matViewProjection;
uniform mat4 vs_matRot;

uniform vec3 vs_v3CameraPos;
uniform vec3 vs_v3LightPos;
uniform vec3 vs_v3InvWavelength;
uniform float vs_fCameraHeight2;
uniform float vs_fOuterRadius;
uniform float vs_fOuterRadius2;
uniform float vs_fInnerRadius;
uniform float vs_fScale;
uniform float vs_fScaleOverScaleDepth;

// POSITION
in vec3 aPosition;

// COLOR0, COLOR1, TEXCOORD0
out vec3 vFrontColor;
out vec3 vSecondaryColor;
out vec3 vDirection;

float scale(float fCos)
{
    float fScaleDepth = 0.25;

    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

void main()
{
    float fKrESun = 0.0375;
    float fKmESun = 0.0225;
    float fKr4PI = 0.0314159;
    float fKm4PI = 0.0188496;
    float fScaleDepth = 0.25;

    // Cg reads the three-component position array as (x, y, z, 1)
    vec4 position = vec4(aPosition, 1.0);

    // Get the ray from the camera to the vertex and its length (which is the far point of the ray
    // passing through the atmosphere)
    vec3 v3Pos = (vs_matRot * position).xyz;
    vec3 v3Ray = v3Pos - vs_v3CameraPos;
    float fFar = length(v3Ray);
    v3Ray /= fFar;

    // Calculate the closest intersection of the ray with the outer atmosphere (which is the near
    // point of the ray passing through the atmosphere)
    float B = 2.0 * dot(vs_v3CameraPos, v3Ray);
    float C = vs_fCameraHeight2 - vs_fOuterRadius2;
    float fDet = max(0.0, B * B - 4.0 * C);
    float fNear = 0.5 * (-B - sqrt(fDet));

    // Calculate the ray's starting position, then calculate its scattering offset
    vec3 v3Start = vs_v3CameraPos + v3Ray * fNear;
    fFar -= fNear;
    float fStartAngle = dot(v3Ray, v3Start) / vs_fOuterRadius;
    float fStartDepth = exp(-1.0 / fScaleDepth);
    float fStartOffset = fStartDepth * scale(fStartAngle);

    // Initialize the scattering loop variables
    float fSampleLength = fFar / 2.0;
    float fScaledLength = fSampleLength * vs_fScale;
    vec3 v3SampleRay = v3Ray * fSampleLength;
    vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

    // Now loop through the sample rays
    vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < 2; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fDepth = exp(vs_fScaleOverScaleDepth * (vs_fInnerRadius - fHeight));
        float fLightAngle = dot(vs_v3LightPos, v3SamplePoint) / fHeight;
        float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
        float fScatter = (fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)));
        vec3 v3Attenuate = exp(-fScatter * (vs_v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }

    // Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel
    // shader. COLOR0 and COLOR1 were OpenGL's primary and secondary colours, which OpenGL clamps to
    // [0, 1].
    vSecondaryColor = clamp(min(v3FrontColor * fKmESun, vec3(100.0, 100.0, 100.0)), 0.0, 1.0);
    vFrontColor = clamp(min(v3FrontColor * (vs_v3InvWavelength * fKrESun), vec3(100.0, 100.0, 100.0)), 0.0, 1.0);

    gl_Position = vs_matViewProjection * position;
    vDirection = vs_v3CameraPos - v3Pos;
}
