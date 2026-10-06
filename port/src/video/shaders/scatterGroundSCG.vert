// GLSL ES 3.00 / GLSL 3.30 translation of harvestClientData/gfx/shaders/scatterGroundSCG.vsh: the
// low shader level's planet transform (no scattering). Vertex-program uniforms carry the prefix vs_.
// The #version line is prepended at run time.

uniform mat4 vs_matViewProjection;
uniform mat4 vs_matWorldInverseTranspose;
uniform mat4 vs_matRot;

// POSITION, TEXCOORD0, NORMAL
in vec3 aPosition;
in vec2 aTexCoord0;
in vec3 aNormal;

// TEXCOORD0, TEXCOORD1
out vec2 vTexCoord0;
out vec4 vNormal;

void main()
{
    // Cg reads the three-component position and normal arrays as (x, y, z, 1)
    gl_Position = vs_matViewProjection * vec4(aPosition, 1.0);
    vTexCoord0 = aTexCoord0;

    vNormal = vs_matWorldInverseTranspose * vec4(aNormal, 1.0);
}
