// OpenGL 1.x's fixed-function vertex stage as the original renderer used it: modelview and
// projection matrices, per-vertex lighting without GL_COLOR_MATERIAL, GL_NORMALIZE or a local
// viewer, sphere-map texture generation, and the eye distance for fog. The #version line is
// prepended at run time (GLSL ES 3.00 or GLSL 3.30).

in vec3 aPosition;
in vec3 aNormal;
// the bytes of an ox::video::SColor (0xAARRGGBB) in memory: B, G, R, A
in vec4 aColor;
in vec2 aTexCoord0;
in vec2 aTexCoord1;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

uniform bool uLighting;
uniform vec4 uLightModelAmbient;
uniform vec4 uMaterialAmbient;
uniform vec4 uMaterialDiffuse;
uniform vec4 uMaterialSpecular;
uniform vec4 uMaterialEmission;
uniform float uMaterialShininess;
uniform bool uLightEnabled[8];
// eye-space positions, w = 0 for directional lights
uniform vec4 uLightPosition[8];
uniform vec4 uLightAmbient[8];
uniform vec4 uLightDiffuse[8];
uniform vec4 uLightSpecular[8];
// constant, linear, quadratic
uniform vec3 uLightAttenuation[8];

uniform bool uTexGenSphere[2];

out vec4 vColor;
out vec2 vTexCoord0;
out vec2 vTexCoord1;
out float vFogCoord;

vec4 lightVertex(vec3 eye, vec3 normal)
{
    vec4 color = uMaterialEmission + uMaterialAmbient * uLightModelAmbient;
    for (int i = 0; i < 8; ++i)
    {
        if (!uLightEnabled[i])
            continue;

        vec3 toLight;
        float attenuation = 1.0;
        if (uLightPosition[i].w != 0.0)
        {
            toLight = uLightPosition[i].xyz / uLightPosition[i].w - eye;
            float d = length(toLight);
            attenuation = 1.0 / (uLightAttenuation[i].x + uLightAttenuation[i].y * d +
                uLightAttenuation[i].z * d * d);
        }
        else
            toLight = uLightPosition[i].xyz;
        vec3 l = normalize(toLight);

        float nDotL = dot(normal, l);
        vec4 term = uMaterialAmbient * uLightAmbient[i];
        if (nDotL > 0.0)
        {
            term += nDotL * uMaterialDiffuse * uLightDiffuse[i];
            vec3 h = normalize(l + vec3(0.0, 0.0, 1.0));
            float nDotH = max(dot(normal, h), 0.0);
            float specular = uMaterialShininess == 0.0 ? 1.0 : pow(nDotH, uMaterialShininess);
            term += specular * uMaterialSpecular * uLightSpecular[i];
        }
        color += attenuation * term;
    }
    return vec4(clamp(color.rgb, 0.0, 1.0), clamp(uMaterialDiffuse.a, 0.0, 1.0));
}

vec2 sphereMap(vec3 eye, vec3 normal)
{
    vec3 r = reflect(normalize(eye), normal);
    float m = 2.0 * sqrt(r.x * r.x + r.y * r.y + (r.z + 1.0) * (r.z + 1.0));
    return r.xy / m + 0.5;
}

void main()
{
    vec4 eye = uModelView * vec4(aPosition, 1.0);
    gl_Position = uProjection * eye;

    vec3 normal = uNormalMatrix * aNormal;
    vColor = uLighting ? lightVertex(eye.xyz, normal) : aColor.bgra;

    vTexCoord0 = uTexGenSphere[0] ? sphereMap(eye.xyz, normal) : aTexCoord0;
    vTexCoord1 = uTexGenSphere[1] ? sphereMap(eye.xyz, normal) : aTexCoord1;

    vFogCoord = abs(eye.z);
}
