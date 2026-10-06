// Builds the main menu's 3D scene the way CMainMenuState::secondInit (steps 8 to 12) does, on a
// recording video driver, and prints every driver call drawAll makes: transforms, materials,
// lights, draws (with the shader constants CScatterShader sets for each draw), then the picking
// and projection queries the menu uses. The output is the reference the GLES3 renderer can be
// checked against (docs/port/menu-scene.md describes what it shows).
//
//     zig build test-menu_scene -- <game data directory> [shader level 0-2] [width height]
//
// The game data directory is the one holding harvestClientData/ (the device's $GAME_RESOURCES$).
// The planets' rotation animators run on the wall clock, so before each frame the test sets the
// planet rotations to fixed values; the animator itself is checked with a fixed clock.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include "daisy/io/CFileSystem.h"
#include "daisy/video/Null/CVideoNull.h"
#include "harvest/gfx/CScatterShader.h"
#include "ox/core/CLine3d.h"
#include "ox/gui/ICursorControl.h"
#include "ox/io/IFileSystem.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/IBillboardSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ILightSceneNode.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/video/IImage.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/IMaterialRendererServices.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SLight.h"
#include "scene/CSceneNodeAnimatorRotation.h"

namespace daisy {
namespace scene {
ox::scene::ISceneManager* createSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
    ox::gui::ICursorControl* cursorControl);
} // end namespace scene
} // end namespace daisy

using ox::core::CVector3d;
typedef CVector3d<float> vector3df;

namespace {

const char* const MATERIAL_NAMES[] =
{
    "SOLID", "SOLID_2_LAYER", "LIGHTMAP", "LIGHTMAP_ADD", "LIGHTMAP_M2", "LIGHTMAP_M4", "LIGHTMAP_LIGHTING",
    "LIGHTMAP_LIGHTING_M2", "LIGHTMAP_LIGHTING_M4", "SPHERE_MAP", "REFLECTION_2_LAYER", "TRANSPARENT_ADD_COLOR",
    "TRANSPARENT_ALPHA_CHANNEL", "TRANSPARENT_VERTEX_ALPHA", "TRANSPARENT_REFLECTION_2_LAYER"
};
const int BUILT_IN_MATERIAL_COUNT = sizeof(MATERIAL_NAMES) / sizeof(MATERIAL_NAMES[0]);

void printVector(const char* label, const vector3df& v)
{
    printf("%s (%.9g, %.9g, %.9g)", label, v.X, v.Y, v.Z);
}

void printMatrix(const ox::core::CMatrix4& m)
{
    printf("[");
    for (int i = 0; i < 16; ++i)
        printf(i ? " %.9g" : "%.9g", m.M[i]);
    printf("]");
}

void printVertex(const ox::video::S3DVertex& v)
{
    printf("      ");
    printVector("pos", v.Pos);
    printVector(" normal", v.Normal);
    printf(" color %08x uv (%.9g, %.9g)\n", (unsigned int)v.Color.color, v.TCoords.X, v.TCoords.Y);
}

//! FNV-1a over raw bytes, to fingerprint vertex and index data.
unsigned int fnv(const void* data, size_t size, unsigned int hash = 2166136261u)
{
    const unsigned char* p = (const unsigned char*)data;
    for (size_t i = 0; i < size; ++i)
        hash = (hash ^ p[i]) * 16777619u;
    return hash;
}

//! A texture that only knows its name and size.
class CTestTexture : public ox::video::ITexture
{
public:
    explicit CTestTexture(const ox::core::CDimension2d<int>& size) : Size(size) {}

    virtual void* lock() { return 0; }
    virtual void unlock() {}
    virtual const ox::core::CDimension2d<int>& getOriginalSize() { return Size; }
    virtual const ox::core::CDimension2d<int>& getSize() { return Size; }
    virtual int getDriverType() { return 0; }
    virtual int getColorFormat() { return ox::video::ECF_A8R8G8B8; }
    virtual int getPitch() { return Size.Width * 4; }

private:
    ox::core::CDimension2d<int> Size;
};

//! The built-in materials, transparent as in daisy's OpenGL driver (the TRANSPARENT_* types).
class CTestMaterialRenderer : public ox::video::IMaterialRenderer
{
public:
    explicit CTestMaterialRenderer(bool transparent) : Transparent(transparent) {}
    virtual bool isTransparent() { return Transparent; }

private:
    bool Transparent;
};

//! A Cg shader material: the shader files, the base material and the constants callback.
struct SCgMaterial
{
    std::string Vertex;
    std::string Pixel;
    ox::video::E_MATERIAL_TYPE Base;
    ox::video::IShaderConstantSetCallBack* Callback;
    int UserData;
};

//! Prints the constants a callback sets.
class CTestServices : public ox::video::IMaterialRendererServices
{
public:
    explicit CTestServices(ox::video::IVideoDriver* driver) : Driver(driver) {}

    virtual void setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates) {}
    virtual bool setVertexShaderConstant(const char* name, const float* floats, int count)
    {
        print("vs", name, floats, count);
        return true;
    }
    virtual void setVertexShaderConstant(const float* data, int startRegister, int constantAmount) {}
    virtual bool setPixelShaderConstant(const char* name, const float* floats, int count)
    {
        print("ps", name, floats, count);
        return true;
    }
    virtual void setPixelShaderConstant(const float* data, int startRegister, int constantAmount) {}
    virtual ox::video::IVideoDriver* getVideoDriver() { return Driver; }

private:
    void print(const char* stage, const char* name, const float* floats, int count)
    {
        printf("      %s %s =", stage, name);
        for (int i = 0; i < count; ++i)
            printf(" %.9g", floats[i]);
        printf("\n");
    }

    ox::video::IVideoDriver* Driver;
};

//! The null driver with the built-in material renderers and Cg materials registered, keeping the
//! transforms, and printing every call the scene manager makes.
class CRecordingDriver : public daisy::video::CVideoNull
{
public:
    CRecordingDriver(ox::io::IFileSystem* fs, const ox::core::CDimension2d<int>& screenSize)
        : CVideoNull(fs, screenSize), Services(this), Recording(false)
    {
        for (int i = 0; i < BUILT_IN_MATERIAL_COUNT; ++i)
            addAndDropMaterialRenderer(new CTestMaterialRenderer(i >= ox::video::EMT_TRANSPARENT_ADD_COLOR));
    }

    void setRecording(bool recording) { Recording = recording; }

    virtual ox::video::ITexture* getTexture(const char* filename)
    {
        ox::video::ITexture* texture = CVideoNull::getTexture(filename);
        if (texture)
        {
            const char* slash = strrchr(filename, '/');
            TextureNames[texture] = slash ? slash + 1 : filename;
        }
        return texture;
    }

    virtual ox::video::ITexture* addTexture(const char* name, ox::video::IImage* image)
    {
        ox::video::ITexture* texture = CVideoNull::addTexture(name, image);
        if (texture)
            TextureNames[texture] = name;
        return texture;
    }

    virtual void setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat)
    {
        static const char* const names[] = { "VIEW", "WORLD", "PROJECTION" };
        Transforms[state] = mat;
        if (!Recording)
            return;
        printf("    setTransform %s ", names[state]);
        printMatrix(mat);
        printf("\n");
    }

    virtual ox::core::CMatrix4 getTransform(ox::video::E_TRANSFORMATION_STATE state)
    {
        return Transforms[state];
    }

    virtual void setMaterial(const ox::video::SMaterial& material)
    {
        Material = material;
        if (!Recording)
            return;
        printf("    setMaterial %s tex0 %s tex1 %s lighting %d zbuffer %d zwrite %d backfaceCulling %d frontFaceCCW %d "
            "bilinear %d\n",
            materialName(material.MaterialType).c_str(), textureName(material.Texture1).c_str(),
            textureName(material.Texture2).c_str(), material.Lighting, material.ZBuffer, material.ZWriteEnable,
            material.BackfaceCulling, material.FrontFaceCCW, material.BilinearFilter);
    }

    virtual void drawIndexedTriangleList(const ox::video::S3DVertex* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount)
    {
        if (!Recording)
            return;
        printf("    drawIndexedTriangleList vertices %d triangles %d indices", vertexCount, triangleCount);
        for (int i = 0; i < triangleCount * 3; ++i)
            printf(" %d", indexList[i]);
        printf("\n");
        for (int i = 0; i < vertexCount; ++i)
            printVertex(vertices[i]);
        setShaderConstants();
    }

    virtual void drawMeshBuffer(ox::scene::IMeshBuffer* mb)
    {
        if (!Recording)
            return;
        const ox::video::S3DVertex* vertices = (const ox::video::S3DVertex*)mb->getVertices();
        printf("    drawMeshBuffer vertices %d indices %d hash %08x/%08x\n", mb->getVertexCount(),
            mb->getIndexCount(), fnv(vertices, mb->getVertexCount() * sizeof(ox::video::S3DVertex)),
            fnv(mb->getIndices(), mb->getIndexCount() * sizeof(unsigned short)));
        setShaderConstants();
    }

    virtual void deleteAllDynamicLights()
    {
        if (Recording)
            printf("    deleteAllDynamicLights\n");
    }

    virtual void addDynamicLight(const ox::video::SLight& light)
    {
        if (!Recording)
            return;
        printf("    addDynamicLight");
        printVector(" position", light.Position);
        printf(" radius %.9g diffuse (%.9g, %.9g, %.9g, %.9g) specular (%.9g, %.9g, %.9g, %.9g) ambient "
            "(%.9g, %.9g, %.9g, %.9g)\n",
            light.Radius, light.DiffuseColor.r, light.DiffuseColor.g, light.DiffuseColor.b, light.DiffuseColor.a,
            light.SpecularColor.r, light.SpecularColor.g, light.SpecularColor.b, light.SpecularColor.a,
            light.AmbientColor.r, light.AmbientColor.g, light.AmbientColor.b, light.AmbientColor.a);
    }

    virtual void setAmbientLight(const ox::video::SColorf& color)
    {
        printf("setAmbientLight (%.9g, %.9g, %.9g, %.9g)\n", color.r, color.g, color.b, color.a);
    }

    virtual void* getGPUProgrammingServices()
    {
        return static_cast<ox::video::IGPUProgrammingServices*>(this);
    }

    //! Registers a Cg material over its base material's renderer, as COpenGLCGMaterialRenderer does.
    virtual int addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
        const char* pixelShaderEntryPointName, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, bool flag, int userData)
    {
        SCgMaterial cg;
        cg.Vertex = strrchr(vertexShaderProgramFileName, '/') + 1;
        cg.Pixel = strrchr(pixelShaderProgramFileName, '/') + 1;
        cg.Base = baseMaterial;
        cg.Callback = callback;
        cg.UserData = userData;

        int id = addAndDropMaterialRenderer(new CTestMaterialRenderer(getMaterialRenderer(baseMaterial)->isTransparent()));
        CgMaterials[id] = cg;
        printf("addCgShaderMaterialFromFiles %s %s base %s -> %d\n", cg.Vertex.c_str(), cg.Pixel.c_str(),
            MATERIAL_NAMES[baseMaterial], id);
        return id;
    }

private:
    std::string materialName(int type)
    {
        if (type >= 0 && type < BUILT_IN_MATERIAL_COUNT)
            return MATERIAL_NAMES[type];
        std::map<int, SCgMaterial>::iterator it = CgMaterials.find(type);
        if (it == CgMaterials.end())
            return "?";
        return "cg:" + it->second.Vertex + "/" + it->second.Pixel + "(" + MATERIAL_NAMES[it->second.Base] + ")";
    }

    std::string textureName(ox::video::ITexture* texture)
    {
        if (!texture)
            return "-";
        std::map<ox::video::ITexture*, std::string>::iterator it = TextureNames.find(texture);
        return it == TextureNames.end() ? "?" : it->second;
    }

    //! The Cg material renderer's OnRender: the callback sets the constants before each draw.
    void setShaderConstants()
    {
        std::map<int, SCgMaterial>::iterator it = CgMaterials.find(Material.MaterialType);
        if (it != CgMaterials.end())
            it->second.Callback->OnSetConstants(&Services, it->second.UserData);
    }

protected:
    virtual ox::video::ITexture* createDeviceDependentTexture(ox::video::IImage* surface)
    {
        return new CTestTexture(surface->getDimension());
    }

private:
    CTestServices Services;
    bool Recording;
    ox::core::CMatrix4 Transforms[ox::video::ETS_COUNT];
    ox::video::SMaterial Material;
    std::map<ox::video::ITexture*, std::string> TextureNames;
    std::map<int, SCgMaterial> CgMaterials;
};

// CMainMenuState's constants.
const int ID_FIRST_PLANET = 1458;
const int ID_ATRUM = 1461;
const int PLANET_COUNT = 3;
const vector3df PLANET_POSITIONS[] =
{
    vector3df(-250.0f, 0.0f, 0.0f), vector3df(-200.0f, 20.0f, 80.0f), vector3df(-150.0f, -10.0f, -30.0f)
};
const vector3df ATRUM_POSITION(-900.0f, -35.0f, 25.0f);
const vector3df SUN_POSITION(800.0f, 0.0f, 0.0f);
const ox::core::CDimension2d<float> SUN_OUTER_FLARE_SIZE(500.0f, 500.0f);
const char* const PLANET_TEXTURES[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephNoShader.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosNoShader.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresNoShader.jpg"
};
const char* const PLANET_DIFF_SPEC_TEXTURES[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephDiffSpec.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosDiffSpec.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresDiffSpec.tga"
};
const char* const PLANET_NORM_GLOW_TEXTURES[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephNormGlow.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosNormGlow.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresNormGlow.tga"
};
const char* const SKYBOX_TEXTURES[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxRoof.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxNorth.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxWest.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxEast.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxSouth.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxFloor.jpg"
};

void printMesh(const char* name, ox::scene::IAnimatedMesh* mesh)
{
    ox::scene::IMesh* m = mesh->getMesh(0);
    ox::scene::IMeshBuffer* mb = m->getMeshBuffer(0);
    const ox::video::S3DVertex* vertices = (const ox::video::S3DVertex*)mb->getVertices();
    const ox::core::CAabbox3d<float>& box = mesh->getBoundingBox();

    printf("mesh %s: frames %d buffers %d vertices %d indices %d hash %08x/%08x", name, mesh->getFrameCount(),
        m->getMeshBufferCount(), mb->getVertexCount(), mb->getIndexCount(),
        fnv(vertices, mb->getVertexCount() * sizeof(ox::video::S3DVertex)),
        fnv(mb->getIndices(), mb->getIndexCount() * sizeof(unsigned short)));
    printVector(" box", box.MinEdge);
    printVector(" -", box.MaxEdge);
    printf("\n    first triangle (indices %d %d %d):\n", mb->getIndices()[0], mb->getIndices()[1], mb->getIndices()[2]);
    for (int i = 0; i < 3; ++i)
        printVertex(vertices[mb->getIndices()[i]]);
}

} // end anonymous namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <game data directory> [shader level 0-2] [width height]\n", argv[0]);
        return 2;
    }
    int shaderLevel = argc > 2 ? atoi(argv[2]) : 2;
    ox::core::CDimension2d<int> screenSize(1024, 768);
    if (argc > 4)
        screenSize = ox::core::CDimension2d<int>(atoi(argv[3]), atoi(argv[4]));

    ox::io::IFileSystem* fs = daisy::io::createFileSystem();
    fs->addDirectoryAlias("$GAME_RESOURCES$", argv[1]);
    CRecordingDriver* driver = new CRecordingDriver(fs, screenSize);
    ox::scene::ISceneManager* smgr = daisy::scene::createSceneManager(driver, fs, 0);

    printf("# menu scene: shader level %d, screen %dx%d\n", shaderLevel, screenSize.Width, screenSize.Height);

    // Step 8: meshes, planets, atmospheres, animators, Atrum.
    std::string atmospherePath = std::string(fs->getDirectoryFromAlias("$GAME_RESOURCES$")) +
        "/harvestClientData/gfx/atmoSphere_48.obj";
    ox::scene::IAnimatedMesh* planetMesh = smgr->getMesh("$GAME_RESOURCES$/harvestClientData/gfx/planetSphere_24.obj");
    ox::scene::IAnimatedMesh* atmosphereMesh = smgr->getMesh(atmospherePath.c_str());
    if (!planetMesh || !atmosphereMesh)
    {
        fprintf(stderr, "cannot load the sphere meshes from %s\n", argv[1]);
        return 1;
    }
    printMesh("planetSphere_24.obj", planetMesh);
    printMesh("atmoSphere_48.obj", atmosphereMesh);

    ox::scene::IAnimatedMeshSceneNode* planetNodes[PLANET_COUNT * 2] = { 0 };
    for (int i = 0; i < PLANET_COUNT; ++i)
    {
        planetNodes[i * 2] = smgr->addAnimatedMeshSceneNode(planetMesh, 0, ID_FIRST_PLANET + i);
        if (shaderLevel == 0)
            planetNodes[i * 2]->setMaterialTexture(0, driver->getTexture(PLANET_TEXTURES[i]));
        else
        {
            planetNodes[i * 2]->setMaterialTexture(0, driver->getTexture(PLANET_DIFF_SPEC_TEXTURES[i]));
            planetNodes[i * 2]->setMaterialTexture(1, driver->getTexture(PLANET_NORM_GLOW_TEXTURES[i]));
            if (shaderLevel == 2)
            {
                planetNodes[i * 2 + 1] = smgr->addAnimatedMeshSceneNode(atmosphereMesh, planetNodes[i * 2],
                    ID_FIRST_PLANET + i);
                planetNodes[i * 2 + 1]->setMaterialType(ox::video::EMT_TRANSPARENT_ADD_COLOR);
                planetNodes[i * 2 + 1]->setVisible(false);
            }
        }
        planetNodes[i * 2]->setMaterialType(ox::video::EMT_SOLID);
        planetNodes[i * 2]->setPosition(PLANET_POSITIONS[i]);

        ox::scene::ISceneNodeAnimator* animator =
            smgr->createRotationAnimator(vector3df(0.0f, (i + 5) * -0.001f, 0.0f));
        planetNodes[i * 2]->addAnimator(animator);
        animator->drop();
    }

    ox::scene::IAnimatedMeshSceneNode* atrum = smgr->addAnimatedMeshSceneNode(planetMesh, 0, ID_ATRUM);
    atrum->setMaterialTexture(0, driver->getTexture("$GAME_RESOURCES$/harvestClientData/gfx/skyboxRoof.jpg"));
    atrum->setMaterialType(ox::video::EMT_SOLID);
    atrum->setPosition(ATRUM_POSITION);

    // Step 9: the camera at its starting point.
    ox::scene::ICameraSceneNode* camera = smgr->addCameraSceneNode();
    smgr->setActiveCamera(camera);
    camera->setPosition(vector3df(-200.0f, -20.0f, -150.0f));
    camera->setTarget(vector3df(-350.0f, 0.0f, -70.0f));
    printf("camera fov %.9g aspect %.9g near %.9g far %.9g projection ", camera->getFOV(), camera->getAspectRatio(),
        camera->getNearValue(), camera->getFarValue());
    printMatrix(camera->getProjectionMatrix());
    printf("\n");

    // Step 10: the light and the scattering shaders.
    driver->setAmbientLight(ox::video::SColorf(ox::video::SColor(0, 32, 32, 32)));
    ox::scene::ILightSceneNode* light = smgr->addLightSceneNode(0, SUN_POSITION, ox::video::SColorf(1.0f, 1.0f, 1.0f, 1.0f),
        100.0f);
    light->getLightData().Radius = 7500.0f;
    light->getLightData().DiffuseColor = ox::video::SColorf(ox::video::SColor(0, 117, 117, 117));

    std::vector<harvest::gfx::CScatterShader*> shaders;
    if (shaderLevel != 0)
        for (int i = 0; i < PLANET_COUNT * 2; ++i)
        {
            if (shaderLevel == 1 && i % 2)
                continue;
            harvest::gfx::CScatterShader* shader = new harvest::gfx::CScatterShader(driver, camera, planetNodes[i]);
            if (i % 2)
                shader->initAtmo();
            else
                shader->initGround(shaderLevel == 2);
            shaders.push_back(shader);
        }

    // Step 11: the skybox (top, bottom, left, right, front, back = roof, floor, east, west, south, north).
    ox::video::ITexture* top = driver->getTexture(SKYBOX_TEXTURES[0]);
    ox::video::ITexture* north = driver->getTexture(SKYBOX_TEXTURES[1]);
    ox::video::ITexture* west = driver->getTexture(SKYBOX_TEXTURES[2]);
    ox::video::ITexture* east = driver->getTexture(SKYBOX_TEXTURES[3]);
    ox::video::ITexture* south = driver->getTexture(SKYBOX_TEXTURES[4]);
    ox::video::ITexture* bottom = driver->getTexture(SKYBOX_TEXTURES[5]);
    ox::scene::ISceneNode* skyBox = smgr->addSkyBoxSceneNode(top, bottom, east, west, south, north);

    // Step 12: the sun.
    ox::scene::IBillboardSceneNode* sun = smgr->addBillboardSceneNode(0, SUN_OUTER_FLARE_SIZE, SUN_POSITION);
    sun->setMaterialType(ox::video::EMT_TRANSPARENT_ADD_COLOR);
    sun->getMaterial(0).Lighting = false;
    sun->getMaterial(0).ZBuffer = false;
    sun->setMaterialTexture(0, driver->getTexture("$GAME_RESOURCES$/harvestClientData/gfx/particlewhite.jpg"));

    // Frames: the first registers the nodes with their constructor-time transformations (at the
    // origin) against the camera's default frustum, so nothing is culled and the transparent
    // distances tie; the second is the menu's opening view (Ares is off screen and culled); the
    // third is a camera next to Poseidon looking past it, as after picking it.
    struct SFrame
    {
        vector3df Position;
        vector3df Target;
        float Rotation;
    };
    const SFrame frames[] =
    {
        { vector3df(-200.0f, -20.0f, -150.0f), vector3df(-350.0f, 0.0f, -70.0f), 0.0f },
        { vector3df(-200.0f, -20.0f, -150.0f), vector3df(-350.0f, 0.0f, -70.0f), 0.0f },
        { PLANET_POSITIONS[1] + vector3df(13.0f, 0.0f, -10.0f), PLANET_POSITIONS[1] + vector3df(13.0f, 0.0f, 0.0f), 30.0f },
    };
    for (size_t f = 0; f < sizeof(frames) / sizeof(frames[0]); ++f)
    {
        camera->setPosition(frames[f].Position);
        camera->setTarget(frames[f].Target);
        for (int i = 0; i < PLANET_COUNT; ++i)
            planetNodes[i * 2]->setRotation(vector3df(0.0f, frames[f].Rotation * (i + 5) * -0.1f, 0.0f));

        printf("frame %d:", (int)f + 1);
        printVector(" camera", frames[f].Position);
        printVector(" target", frames[f].Target);
        printf(" planet rotation y %.9g, %.9g, %.9g\n", frames[f].Rotation * -0.5f, frames[f].Rotation * -0.6f,
            frames[f].Rotation * -0.7f);
        driver->setRecording(true);
        smgr->drawAll();
        driver->setRecording(false);
    }

    // Picking and projection at the frame 2 view, as the hover and popup code use them.
    camera->setPosition(frames[1].Position);
    camera->setTarget(frames[1].Target);
    smgr->drawAll();
    printf("picking:\n");
    ox::scene::ISceneCollisionManager* collision = smgr->getSceneCollisionManager();
    for (int id = ID_FIRST_PLANET; id <= ID_ATRUM; ++id)
    {
        ox::scene::ISceneNode* node = smgr->getSceneNodeFromId(id);
        ox::core::CPosition2d<int> screen = collision->getScreenCoordinatesFrom3DPosition(node->getAbsolutePosition());
        ox::scene::ISceneNode* hit = collision->getSceneNodeFromScreenCoordinatesBB(screen);
        printf("  node %d", id);
        printVector(" at", node->getAbsolutePosition());
        printf(" -> screen (%d, %d) -> picked %d\n", screen.X, screen.Y, hit ? hit->getID() : 0);
    }
    ox::core::CPosition2d<int> corner(0, 0);
    ox::scene::ISceneNode* cornerHit = collision->getSceneNodeFromScreenCoordinatesBB(corner);
    printf("  screen (0, 0) -> picked %s\n", cornerHit ? "a node" : "nothing");
    ox::core::CLine3d<float> ray = collision->getRayFromScreenCoordinates(
        ox::core::CPosition2d<int>(screenSize.Width / 2, screenSize.Height / 2), 0);
    printVector("  center ray", ray.start);
    printVector(" -", ray.end);
    printf("\n");
    ox::core::CPosition2d<int> behind = collision->getScreenCoordinatesFrom3DPosition(vector3df(-200.0f, -20.0f, -300.0f));
    printf("  point behind the camera -> (%d, %d)\n", behind.X, behind.Y);

    // The rotation animator with a fixed clock: 1000 ms at -0.005 per 10 ms.
    ox::scene::ISceneNode* probe = smgr->addBillboardSceneNode();
    daisy::scene::CSceneNodeAnimatorRotation* animator =
        new daisy::scene::CSceneNodeAnimatorRotation(5000, vector3df(0.0f, -0.005f, 0.0f));
    animator->animateNode(probe, 6000);
    animator->animateNode(probe, 6500);
    printVector("animator: rotation after 1500 ms", probe->getRotation());
    printf("\n");
    animator->drop();

    // Teardown as in the CMainMenuState destructor.
    for (size_t i = 0; i < shaders.size(); ++i)
        shaders[i]->drop();
    skyBox->remove();
    smgr->clear();
    printf("after clear: root children %d, active camera %s\n", (int)smgr->getRootSceneNode()->getChildren().size(),
        smgr->getActiveCamera() ? "set" : "none");

    smgr->drop();
    driver->drop();
    fs->drop();
    return 0;
}
