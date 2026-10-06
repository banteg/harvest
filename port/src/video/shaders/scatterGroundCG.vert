// GLSL ES 3.00 / GLSL 3.30 translation of harvestClientData/gfx/shaders/scatterGroundCG.vsh:
// Sean O'Neil's atmospheric scattering, ground from space (GPU Gems 2, chapter 16; copyright (c)
// 2004 Sean O'Neil). Vertex-program uniforms carry the prefix vs_. The #version line is prepended
// at run time.

uniform mat4 vs_matViewProjection;
uniform mat4 vs_matRot;
uniform mat4 vs_matWorldInverseTranspose;

uniform vec3 vs_v3CameraPos;
uniform vec3 vs_v3LightPos;
uniform vec3 vs_v3InvWavelength;
uniform float vs_fCameraHeight2;
uniform float vs_fOuterRadius;
uniform float vs_fOuterRadius2;
uniform float vs_fInnerRadius;
uniform float vs_fScale;
uniform float vs_fScaleOverScaleDepth;

// POSITION, TEXCOORD0, NORMAL
in vec3 aPosition;
in vec2 aTexCoord0;
in vec3 aNormal;

// COLOR0, COLOR1, TEXCOORD0, TEXCOORD1
out vec4 vFrontColor;
out vec4 vSecondaryColor;
out vec2 vTexCoord;
out vec4 vNormal;

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

    // Cg reads the three-component position and normal arrays as (x, y, z, 1)
    vec4 position = vec4(aPosition, 1.0);
    vec4 normal = vec4(aNormal, 1.0);

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
    float fDepth = exp((vs_fInnerRadius - vs_fOuterRadius) / fScaleDepth);
    float fCameraAngle = dot(-v3Ray, v3Pos) / length(v3Pos);
    float fLightAngle = dot(vs_v3LightPos, v3Pos) / length(v3Pos);
    float fCameraScale = scale(fCameraAngle);
    float fLightScale = scale(fLightAngle);
    float fCameraOffset = fDepth * fCameraScale;
    float fTemp = (fLightScale + fCameraScale);

    // Initialize the scattering loop variables
    float fSampleLength = fFar / 2.0;
    float fScaledLength = fSampleLength * vs_fScale;
    vec3 v3SampleRay = v3Ray * fSampleLength;
    vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

    // Now loop through the sample rays (the Cg source reuses the name fDepth inside the loop)
    vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);
    vec3 v3Attenuate;
    for (int i = 0; i < 2; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fSampleDepth = exp(vs_fScaleOverScaleDepth * (vs_fInnerRadius - fHeight));
        float fScatter = fSampleDepth * fTemp - fCameraOffset;
        v3Attenuate = exp(-fScatter * (vs_v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fSampleDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }

    // COLOR0 and COLOR1 were OpenGL's primary and secondary colours, which OpenGL clamps to [0, 1].
    vFrontColor = clamp(vec4(v3FrontColor * (vs_v3InvWavelength * fKrESun + fKmESun), 1.0), 0.0, 1.0);

    // Calculate the attenuation factor for the ground
    vSecondaryColor = clamp(vec4(v3Attenuate, 1.0), 0.0, 1.0);

    gl_Position = vs_matViewProjection * position;
    vTexCoord = aTexCoord0;

    vNormal = vs_matWorldInverseTranspose * normal;
}
