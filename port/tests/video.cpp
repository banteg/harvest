// A standalone test of the renderer (port/src/video/): opens an SDL3 window with an OpenGL 3.3 core
// (or, with --gles, OpenGL ES 3.0) context, creates the driver through port::createVideoDriver, and
// draws three scenes from the original game data:
// - 2D: the sprites of harvestMenu.dat and two fonts, every draw2DImage variant, rectangles, lines
//   and a scissor rectangle;
// - 3D: the main menu's three planet materials (the Cg scattering shaders through their GLSL
//   translations, and EMT_SOLID), an atmosphere shell and an additive billboard, under a 2D overlay.
//   The scattering constants are set by a copy of CScatterShader::OnSetConstants over hand-made
//   meshes, so this scene needs nothing but the driver;
// - menu: the main menu's scene as CMainMenuState builds it (tests/menu_scene.cpp), on the port's
//   scene manager with the real CScatterShader, at --level 0, 1 or 2 (default 2), from the menu's
//   opening camera (and, every other two seconds, the camera after picking Poseidon).
//
//     zig build test-video -- <data directory> [--screenshot <directory>] [--frames <n>]
//         [--scene 2d|3d|menu|all] [--level 0-2] [--gles] [--size WxH] [--scale <factor>]
//
// --size is the screen size the driver gets (default 1024x768) and --scale the drawable pixels per
// screen unit (default 1), as the game's device sets them on a high-density display: with --scale 2
// the scenes are laid out for 1024x768 and drawn into a 2048x1536 drawable.
//
// The data directory is the one holding harvestClientData (orig/1.18-linux-amd64). With
// --screenshot each scene is saved through IVideoDriver::saveJpegScreenshot as
// <directory>/video-test-2d-<yymmdd>-NN.jpg (and -3d-, -menu-) after --frames frames (default 3),
// and the program exits; otherwise it runs until the window is closed, and space switches the
// scene.

#define SDL_MAIN_HANDLED 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "daisy/gui/BuildInFont.h"
#include "daisy/io/CFileSystem.h"
#include "daisy/io/CMemoryReadFile.h"
#include "ox/io/IReadFile.h"
#include "daisy/os.h"
#include "daisy/other/CLogger.h"
#include "ox/IOxDevice.h"
#include "ox/core/CMatrix4.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IGPUProgrammingServices.h"
#include "ox/video/IMaterialRendererServices.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertexInline.h"
#include "ox/video/SColorArray.h"
#include "ox/video/SLight.h"
#include "ox/video/SMaterial.h"
#include "harvest/gfx/CScatterShader.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/IBillboardSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ILightSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "platform/Seams.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace daisy {
namespace scene {
//! The scene manager's factory (port/src/scene/), declared by CIrrDeviceStub.cpp.
ox::scene::ISceneManager* createSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
    ox::gui::ICursorControl* cursorControl);
} // end namespace scene
} // end namespace daisy

using namespace ox;
using ox::core::CDimension2d;
using ox::core::CMatrix4;
using ox::core::CPosition2d;
using ox::core::CRect;
using ox::core::CVector3d;
using ox::video::SColor;

namespace {

//! The device the driver presents through: only swapBuffers does anything.
class CTestDevice : public IOxDevice
{
public:
    CTestDevice(SDL_Window* window) : Window(window) {}

    virtual bool createDeviceWindow(const core::CDimension2d<int>&, unsigned int, bool, bool, bool, unsigned int)
    {
        return false;
    }
    virtual bool createUserSelectedDeviceWindow(const TArray<core::CString<wchar_t> >*, unsigned int) { return false; }
    virtual int getSelectedLanguageIndex() { return 0; }
    virtual bool setVideoDriver(video::E_DRIVER_TYPE) { return false; }
    virtual bool run() { return true; }
    virtual bool swapBuffers() { return SDL_GL_SwapWindow(Window); }
    virtual video::IVideoDriver* getVideoDriver() { return 0; }
    virtual bool setFullscreenMode(bool) { return false; }
    virtual void resizeDeviceWindow(const core::CDimension2d<int>&) {}
    virtual io::IFileSystem* getFileSystem() { return 0; }
    virtual gui::IGUIEnvironment* getGUIEnvironment() { return 0; }
    virtual scene::ISceneManager* getSceneManager() { return 0; }
    virtual gui::ICursorControl* getCursorControl() { return 0; }
    virtual event::ILogger* getLogger() { return 0; }
    virtual video::IVideoModeList* getVideoModeList() { return 0; }
    virtual IOSOperator* getOSOperator() { return 0; }
    virtual ITimer* getTimer() { return 0; }
    virtual void setWindowCaption(const wchar_t*) {}
    virtual void moveWindow(int, int) {}
    virtual bool isWindowActive() { return true; }
    virtual void closeDevice() {}
    virtual const wchar_t* getVersion() { return L"0.7"; }
    virtual void setEventReceiver(event::IEventReceiver*) {}
    virtual void setAcceptsDragAndDrop(event::E_DRAG_TYPE, bool) {}
    virtual bool getAcceptsDragAndDrop(event::E_DRAG_TYPE) { return false; }
    virtual void setAcceptsDragAndDropFileType(const char*, bool) {}
    virtual bool getAcceptsDragAndDropFileType(const char*) { return false; }
    virtual void setResizeAble(bool) {}
    virtual void createGUIAndScene() {}
    virtual net::INetworkDevice* createNetworkDevice(const char*, int) { return 0; }
    virtual net::INetworkDevice* getNetworkDevice(const char*) { return 0; }
    virtual void removeNetworkDevice(const char*) {}
    virtual void pollNetworkDevices() {}
    virtual audio::IAudioDriver* getAudioDriver() { return 0; }
    virtual input::IJoystickDriver* getJoystickDriver() { return 0; }
    virtual void initThreadPool(unsigned int) {}
    virtual IThreadPool* getThreadPool() { return 0; }

private:
    SDL_Window* Window;
};

//! A planet or atmosphere: what CScatterShader reads from its scene node and camera.
struct SNode
{
    CVector3d<float> Position;
    float Angle;
    video::SMaterial Material;
    std::vector<video::S3DVertex> Vertices;
    std::vector<unsigned short> Indices;

    CMatrix4 getAbsoluteTransformation() const
    {
        // rotation about +y, then the translation
        CMatrix4 m;
        const float c = cosf(Angle), s = sinf(Angle);
        m.M[0] = c;
        m.M[2] = -s;
        m.M[8] = s;
        m.M[10] = c;
        m.setTranslation(Position);
        return m;
    }
};

//! CScatterShader::OnSetConstants (src/HarvestFull/harvest/gfx/CScatterShader.cpp) over SNode.
class CScatterConstants : public video::IShaderConstantSetCallBack
{
public:
    CScatterConstants(video::IVideoDriver* driver, const CVector3d<float>* camera, const SNode* node, bool ground,
        bool scattering)
        : Driver(driver), Camera(camera), Node(node), Ground(ground), Scattering(scattering)
    {
    }

    virtual void OnSetConstants(video::IMaterialRendererServices* services, int userData)
    {
        CVector3d<float> cameraPos = *Camera - Node->Position;
        CVector3d<float> lightPos = (-Node->Position).normalize();
        float invWavelength[3] = {5.60204601f, 9.47328377f, 19.6438046f};
        float cameraHeight2 = cameraPos.getLengthSQ();
        float innerRadius = 10.0f;
        float outerRadius = 10.25f;
        float outerRadius2 = outerRadius * outerRadius;
        float scale = 1.0f / (outerRadius - innerRadius);
        float scaleOverScaleDepth = scale / 0.25f;

        CMatrix4 translation;
        translation.setTranslation(-Node->Position);
        CMatrix4 matRot = translation * Node->getAbsoluteTransformation();

        CMatrix4 matViewProjection = Driver->getTransform(video::ETS_PROJECTION);
        matViewProjection *= Driver->getTransform(video::ETS_VIEW);
        matViewProjection *= Driver->getTransform(video::ETS_WORLD);

        CMatrix4 matWorldInverseTranspose;
        if (Ground)
        {
            CMatrix4 inverse;
            matRot.getInverse(inverse);
            matWorldInverseTranspose = inverse.getTransposed();
        }

        if (Scattering)
        {
            services->setVertexShaderConstant("v3CameraPos", &cameraPos.X, 3);
            services->setVertexShaderConstant("v3LightPos", &lightPos.X, 3);
            services->setVertexShaderConstant("v3InvWavelength", invWavelength, 3);
            services->setVertexShaderConstant("fCameraHeight2", &cameraHeight2, 1);
            services->setVertexShaderConstant("fInnerRadius", &innerRadius, 1);
            services->setVertexShaderConstant("fOuterRadius", &outerRadius, 1);
            services->setVertexShaderConstant("fOuterRadius2", &outerRadius2, 1);
            services->setVertexShaderConstant("fScale", &scale, 1);
            services->setVertexShaderConstant("fScaleOverScaleDepth", &scaleOverScaleDepth, 1);
        }
        services->setVertexShaderConstant("matRot", matRot.M, 16);
        services->setVertexShaderConstant("matViewProjection", matViewProjection.M, 16);
        if (Ground)
        {
            services->setVertexShaderConstant("matWorldInverseTranspose", matWorldInverseTranspose.M, 16);
            services->setPixelShaderConstant("v3CameraPos", &cameraPos.X, 3);
        }
        services->setPixelShaderConstant("v3LightPos", &lightPos.X, 3);
    }

private:
    video::IVideoDriver* Driver;
    const CVector3d<float>* Camera;
    const SNode* Node;
    bool Ground;
    bool Scattering;
};

//! A UV sphere like planetSphere_24.obj as the OBJ loader leaves it: v = 0 at the south pole, V
//! negated, outward normals, faces counter-clockwise around them (front-facing under GL_CW).
void buildSphere(SNode& node, float radius, int segments, int rings)
{
    for (int r = 0; r <= rings; ++r)
    {
        const float v = (float)r / rings;
        const float theta = (v - 0.5f) * 3.14159265f;
        for (int s = 0; s <= segments; ++s)
        {
            const float u = (float)s / segments;
            const float phi = u * 2.0f * 3.14159265f;
            CVector3d<float> n(cosf(theta) * cosf(phi), sinf(theta), cosf(theta) * sinf(phi));
            node.Vertices.push_back(video::S3DVertex(n.X * radius, n.Y * radius, n.Z * radius, n.X, n.Y, n.Z,
                SColor(0xffffffff), u, -v));
        }
    }
    for (int r = 0; r < rings; ++r)
        for (int s = 0; s < segments; ++s)
        {
            unsigned short a = (unsigned short)(r * (segments + 1) + s);
            unsigned short b = (unsigned short)(a + segments + 1);
            // (a, a+1, b+1) and (a, b+1, b) wind counter-clockwise seen from outside
            unsigned short tri[6] = {a, b, (unsigned short)(b + 1), a, (unsigned short)(b + 1), (unsigned short)(a + 1)};
            node.Indices.insert(node.Indices.end(), tri, tri + 6);
        }
}

//! Irrlicht 0.7's buildProjectionMatrixPerspectiveFovLH.
CMatrix4 perspectiveLH(float fov, float aspect, float zNear, float zFar)
{
    const float h = cosf(fov / 2) / sinf(fov / 2);
    const float w = h / aspect;
    CMatrix4 m;
    memset(m.M, 0, sizeof(m.M));
    m.M[0] = 2 * zNear / w;
    m.M[5] = 2 * zNear / h;
    m.M[10] = zFar / (zFar - zNear);
    m.M[11] = 1;
    m.M[14] = zNear * zFar / (zNear - zFar);
    return m;
}

//! Irrlicht 0.7's buildCameraLookAtMatrixLH.
CMatrix4 lookAtLH(const CVector3d<float>& position, const CVector3d<float>& target, const CVector3d<float>& up)
{
    CVector3d<float> z = target - position;
    z.normalize();
    CVector3d<float> x = up.crossProduct(z);
    x.normalize();
    CVector3d<float> y = z.crossProduct(x);

    CMatrix4 m;
    m.M[0] = x.X;
    m.M[1] = y.X;
    m.M[2] = z.X;
    m.M[3] = 0;
    m.M[4] = x.Y;
    m.M[5] = y.Y;
    m.M[6] = z.Y;
    m.M[7] = 0;
    m.M[8] = x.Z;
    m.M[9] = y.Z;
    m.M[10] = z.Z;
    m.M[11] = 0;
    m.M[12] = -x.dotProduct(position);
    m.M[13] = -y.dotProduct(position);
    m.M[14] = -z.dotProduct(position);
    m.M[15] = 1;
    return m;
}

void drawNode(video::IVideoDriver* driver, const SNode& node)
{
    driver->setTransform(video::ETS_WORLD, node.getAbsoluteTransformation());
    driver->setMaterial(node.Material);
    driver->drawIndexedTriangleList(&node.Vertices[0], (int)node.Vertices.size(), &node.Indices[0],
        (int)node.Indices.size() / 3);
}

//! The font's glyphs by character code, as CUnicodeFont reads them.
struct SFont
{
    video::ISpritePackage* Package;
    std::vector<video::ISpriteAnimationState*> Glyphs;

    void load(video::IVideoDriver* driver, const char* path)
    {
        Package = driver->getSpritePackage(path, true);
        Glyphs.assign(256, 0);
        if (!Package)
            return;
        const TArray<core::CString<char> >& names = Package->getAnimationList();
        for (unsigned int i = 0; i < names.size(); ++i)
        {
            long code = strtol(names[i].c_str(), 0, 10);
            if (code > 0 && code < 256)
                Glyphs[code] = Package->addNewAnimationState(names[i]);
        }
    }

    void draw(const char* text, CPosition2d<int> position, SColor color)
    {
        for (; *text; ++text)
        {
            video::ISpriteAnimationState* glyph = Glyphs[(unsigned char)*text];
            if (!glyph)
            {
                position.X += 8;
                continue;
            }
            glyph->draw(position, 0, color);
            position.X += glyph->getFrameSize(0).Width;
        }
    }
};

//! The main menu's scene as CMainMenuState::secondInit steps 8 to 12 build it (see
//! tests/menu_scene.cpp), on the port's scene manager and this driver.
struct SMenuScene
{
    scene::ISceneManager* SceneManager;
    scene::ICameraSceneNode* Camera;
    std::vector<harvest::gfx::CScatterShader*> Shaders;

    bool build(video::IVideoDriver* driver, io::IFileSystem* fileSystem, int shaderLevel)
    {
        static const CVector3d<float> positions[3] = {CVector3d<float>(-250.0f, 0.0f, 0.0f),
            CVector3d<float>(-200.0f, 20.0f, 80.0f), CVector3d<float>(-150.0f, -10.0f, -30.0f)};
        static const char* const planets[3] = {"Heph", "Pos", "Ares"};
        const char* gfx = "$GAME_RESOURCES$/harvestClientData/gfx/";

        SceneManager = daisy::scene::createSceneManager(driver, fileSystem, 0);
        core::CString<char> path = gfx;
        path.append(core::CString<char>("planetSphere_24.obj"));
        scene::IAnimatedMesh* planetMesh = SceneManager->getMesh(path.c_str());
        path = fileSystem->getDirectoryFromAlias("$GAME_RESOURCES$");
        path.append(core::CString<char>("/harvestClientData/gfx/atmoSphere_48.obj"));
        scene::IAnimatedMesh* atmosphereMesh = SceneManager->getMesh(path.c_str());
        if (!planetMesh || !atmosphereMesh)
            return false;

        scene::IAnimatedMeshSceneNode* nodes[6] = {0};
        for (int i = 0; i < 3; ++i)
        {
            nodes[i * 2] = SceneManager->addAnimatedMeshSceneNode(planetMesh, 0, 1458 + i);
            path = gfx;
            path.append(core::CString<char>("shaders/"));
            path.append(core::CString<char>(planets[i]));
            core::CString<char> name = path;
            if (shaderLevel == 0)
            {
                name.append(core::CString<char>("NoShader.jpg"));
                nodes[i * 2]->setMaterialTexture(0, driver->getTexture(name.c_str()));
            }
            else
            {
                name.append(core::CString<char>("DiffSpec.tga"));
                nodes[i * 2]->setMaterialTexture(0, driver->getTexture(name.c_str()));
                name = path;
                name.append(core::CString<char>("NormGlow.tga"));
                nodes[i * 2]->setMaterialTexture(1, driver->getTexture(name.c_str()));
                if (shaderLevel == 2)
                {
                    nodes[i * 2 + 1] = SceneManager->addAnimatedMeshSceneNode(atmosphereMesh, nodes[i * 2], 1458 + i);
                    nodes[i * 2 + 1]->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
                    nodes[i * 2 + 1]->setVisible(false);
                }
            }
            nodes[i * 2]->setMaterialType(video::EMT_SOLID);
            nodes[i * 2]->setPosition(positions[i]);
            scene::ISceneNodeAnimator* animator =
                SceneManager->createRotationAnimator(CVector3d<float>(0.0f, (i + 5) * -0.001f, 0.0f));
            nodes[i * 2]->addAnimator(animator);
            animator->drop();
        }

        path = gfx;
        path.append(core::CString<char>("skyboxRoof.jpg"));
        scene::IAnimatedMeshSceneNode* atrum = SceneManager->addAnimatedMeshSceneNode(planetMesh, 0, 1461);
        atrum->setMaterialTexture(0, driver->getTexture(path.c_str()));
        atrum->setMaterialType(video::EMT_SOLID);
        atrum->setPosition(CVector3d<float>(-900.0f, -35.0f, 25.0f));

        Camera = SceneManager->addCameraSceneNode();
        SceneManager->setActiveCamera(Camera);

        driver->setAmbientLight(video::SColorf(SColor(0, 32, 32, 32)));
        scene::ILightSceneNode* light = SceneManager->addLightSceneNode(0, CVector3d<float>(800.0f, 0.0f, 0.0f),
            video::SColorf(1.0f, 1.0f, 1.0f, 1.0f), 100.0f);
        light->getLightData().Radius = 7500.0f;
        light->getLightData().DiffuseColor = video::SColorf(SColor(0, 117, 117, 117));

        if (shaderLevel != 0)
            for (int i = 0; i < 6; ++i)
            {
                if (!nodes[i])
                    continue;
                harvest::gfx::CScatterShader* shader = new harvest::gfx::CScatterShader(driver, Camera, nodes[i]);
                if (i % 2)
                    shader->initAtmo();
                else
                    shader->initGround(shaderLevel == 2);
                Shaders.push_back(shader);
            }

        const char* sky[6] = {"skyboxRoof.jpg", "skyboxFloor.jpg", "skyboxEast.jpg", "skyboxWest.jpg",
            "skyboxSouth.jpg", "skyboxNorth.jpg"};
        video::ITexture* textures[6];
        for (int i = 0; i < 6; ++i)
        {
            path = gfx;
            path.append(core::CString<char>(sky[i]));
            textures[i] = driver->getTexture(path.c_str());
        }
        SceneManager->addSkyBoxSceneNode(textures[0], textures[1], textures[2], textures[3], textures[4],
            textures[5]);

        scene::IBillboardSceneNode* sun = SceneManager->addBillboardSceneNode(0, CDimension2d<float>(500.0f, 500.0f),
            CVector3d<float>(800.0f, 0.0f, 0.0f));
        sun->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
        sun->getMaterial(0).Lighting = false;
        sun->getMaterial(0).ZBuffer = false;
        path = gfx;
        path.append(core::CString<char>("particlewhite.jpg"));
        sun->setMaterialTexture(0, driver->getTexture(path.c_str()));
        return true;
    }
};

} // end anonymous namespace

int main(int argc, char** argv)
{
    const char* dataDirectory = 0;
    const char* screenshotDirectory = 0;
    int frames = 3;
    bool gles = false;
    int width = 1024, height = 768;
    float scale = 1.0f;
    // 0: 2D, 1: 3D, 2: the main menu's scene, -1: all of them
    int sceneArgument = -1;
    int shaderLevel = 2;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            screenshotDirectory = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--gles"))
            gles = true;
        else if (!strcmp(argv[i], "--size") && i + 1 < argc)
            sscanf(argv[++i], "%dx%d", &width, &height);
        else if (!strcmp(argv[i], "--scale") && i + 1 < argc)
            scale = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--scene") && i + 1 < argc)
        {
            ++i;
            sceneArgument = !strcmp(argv[i], "2d") ? 0 : !strcmp(argv[i], "3d") ? 1 : !strcmp(argv[i], "menu") ? 2 : -1;
        }
        else if (!strcmp(argv[i], "--level") && i + 1 < argc)
            shaderLevel = atoi(argv[++i]);
        else
            dataDirectory = argv[i];
    }
    if (!dataDirectory || frames < 1 || scale <= 0.0f)
    {
        fprintf(stderr, "usage: harvest-test-video <data directory> [--screenshot <directory>] [--frames <n>] "
                        "[--scene 2d|3d|menu|all] [--level 0-2] [--gles] [--size WxH] [--scale <factor>]\n");
        return 2;
    }

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    if (gles)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    }
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // a window whose drawable is the screen size times the scale, in the display's window coordinates
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    const float density = mode ? mode->pixel_density : 1.0f;
    SDL_Window* window = SDL_CreateWindow("Harvest renderer test", (int)(width * scale / density + 0.5f),
        (int)(height * scale / density + 0.5f), SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
    {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    daisy::CLogger logger(0);
    daisy::os::Printer::Logger = &logger;

    io::IFileSystem* fileSystem = daisy::io::createFileSystem();
    fileSystem->addDirectoryAlias("$GAME_RESOURCES$", dataDirectory);

    // the screen size: the drawable divided by the scale, as the device computes it
    int pixelWidth = width, pixelHeight = height;
    SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight);
    const CDimension2d<int> screenSize((int)(pixelWidth / scale + 0.5f), (int)(pixelHeight / scale + 0.5f));
    printf("screen %dx%d in a %dx%d drawable\n", screenSize.Width, screenSize.Height, pixelWidth, pixelHeight);

    CTestDevice device(window);
    video::IVideoDriver* driver = port::createVideoDriver(&device, fileSystem, screenSize);
    if (!driver)
    {
        fprintf(stderr, "createVideoDriver failed\n");
        return 1;
    }
    // what the game sets at start-up
    driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

    // 2D content
    video::ISpritePackage* menu = driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
        false);
    std::vector<video::ISpriteAnimationState*> sprites;
    if (menu)
    {
        const TArray<core::CString<char> >& names = menu->getAnimationList();
        for (unsigned int i = 0; i < names.size(); ++i)
            sprites.push_back(menu->addNewAnimationState(names[i]));
        printf("harvestMenu.dat: %d animations, %d textures\n", (int)names.size(),
            (int)menu->getTextureList().size());
    }
    SFont font;
    font.load(driver, "$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt");
    io::IReadFile* builtInFontFile = daisy::io::createMemoryReadFile(daisy::gui::BuildInFontData,
        daisy::gui::BuildInFontDataSize, "#DefaultFont", false);
    video::ITexture* builtInFont = driver->getTexture(builtInFontFile);
    builtInFontFile->drop();
    SFont smallFont;
    smallFont.load(driver, "$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");

    // 3D content: the main menu's three shader levels side by side
    const char* shaders = "$GAME_RESOURCES$/harvestClientData/gfx/shaders/";
    core::CString<char> path;
    video::IGPUProgrammingServices* gpu = (video::IGPUProgrammingServices*)driver->getGPUProgrammingServices();
    CVector3d<float> camera(-225.0f, 6.0f, -48.0f);

    SNode planets[3], atmosphere, sun;
    const char* planetNames[3] = {"Heph", "Pos", "Ares"};
    for (int i = 0; i < 3; ++i)
    {
        buildSphere(planets[i], 10.0f, 24, 12);
        planets[i].Position = CVector3d<float>(-250.0f + 25.0f * i, 0.0f, 0.0f);
        planets[i].Angle = 0.0f;
    }
    buildSphere(atmosphere, 10.25f, 48, 24);
    atmosphere.Position = planets[0].Position;
    atmosphere.Angle = 0.0f;

    // high: scatterGroundCG over EMT_SOLID_2_LAYER, with the scatterAtmoCG atmosphere
    CScatterConstants groundHigh(driver, &camera, &planets[0], true, true);
    CScatterConstants groundLow(driver, &camera, &planets[1], true, false);
    CScatterConstants atmo(driver, &camera, &atmosphere, false, true);
    for (int i = 0; i < 2; ++i)
    {
        path = shaders;
        path.append(core::CString<char>(planetNames[i]));
        path.append(core::CString<char>("DiffSpec.tga"));
        planets[i].Material.Texture1 = driver->getTexture(path.c_str());
        path = shaders;
        path.append(core::CString<char>(planetNames[i]));
        path.append(core::CString<char>("NormGlow.tga"));
        planets[i].Material.Texture2 = driver->getTexture(path.c_str());
    }
    path = shaders;
    path.append(core::CString<char>("scatterGroundCG.vsh"));
    core::CString<char> pixelPath = shaders;
    pixelPath.append(core::CString<char>("scatterGroundCG.psh"));
    int groundMaterial = gpu->addCgShaderMaterialFromFiles(path.c_str(), "main", pixelPath.c_str(), "main",
        &groundHigh, video::EMT_SOLID_2_LAYER, false, 0);
    path = shaders;
    path.append(core::CString<char>("scatterGroundSCG.vsh"));
    pixelPath = shaders;
    pixelPath.append(core::CString<char>("scatterGroundSCG.psh"));
    int groundSimpleMaterial = gpu->addCgShaderMaterialFromFiles(path.c_str(), "main", pixelPath.c_str(), "main",
        &groundLow, video::EMT_SOLID_2_LAYER, false, 0);
    path = shaders;
    path.append(core::CString<char>("scatterAtmoCG.vsh"));
    pixelPath = shaders;
    pixelPath.append(core::CString<char>("scatterAtmoCG.psh"));
    int atmoMaterial = gpu->addCgShaderMaterialFromFiles(path.c_str(), "main", pixelPath.c_str(), "main", &atmo,
        video::EMT_TRANSPARENT_ADD_COLOR, false, 0);
    printf("shader materials: ground %d, ground simple %d, atmosphere %d\n", groundMaterial, groundSimpleMaterial,
        atmoMaterial);

    planets[0].Material.MaterialType = groundMaterial != -1 ? (video::E_MATERIAL_TYPE)groundMaterial : video::EMT_SOLID;
    planets[1].Material.MaterialType =
        groundSimpleMaterial != -1 ? (video::E_MATERIAL_TYPE)groundSimpleMaterial : video::EMT_SOLID;
    // none: the shader-less map on EMT_SOLID, lit (GL_DECAL hides the lighting)
    path = shaders;
    path.append(core::CString<char>("AresNoShader.jpg"));
    planets[2].Material.Texture1 = driver->getTexture(path.c_str());
    planets[2].Material.MaterialType = video::EMT_SOLID;
    atmosphere.Material.MaterialType = (video::E_MATERIAL_TYPE)atmoMaterial;
    atmosphere.Material.FrontFaceCCW = true;

    // the skybox (CSkyBoxSceneNode with daisy's texture coordinates, menu-scene.md): six EMT_SOLID
    // faces around the camera, lighting, z-buffer and z-writes off
    SNode skybox[6];
    {
        const char* faceTextures[6] = {"skyboxSouth", "skyboxEast", "skyboxNorth", "skyboxWest", "skyboxRoof",
            "skyboxFloor"};
        // corners as signs of (x, y, z)
        const int corners[6][4][3] = {
            {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}},
            {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},
            {{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}},
            {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},
            {{1, 1, 1}, {-1, 1, 1}, {-1, 1, -1}, {1, 1, -1}},
            {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}},
        };
        const float o = 1.0f / 1536.0f, tt = 1.0f - o;
        const float sides[4][2] = {{tt, tt}, {o, tt}, {o, o}, {tt, o}};
        const float bottom[4][2] = {{o, o}, {tt, o}, {tt, tt}, {o, tt}};
        const unsigned short faceIndices[6] = {0, 1, 2, 0, 2, 3};
        for (int f = 0; f < 6; ++f)
        {
            for (int c = 0; c < 4; ++c)
            {
                const float* uv = f == 5 ? bottom[c] : sides[c];
                skybox[f].Vertices.push_back(video::S3DVertex(corners[f][c][0] * 10.0f, corners[f][c][1] * 10.0f,
                    corners[f][c][2] * 10.0f, 0, 0, 0, SColor(0xffffffff), uv[0], uv[1]));
            }
            skybox[f].Indices.assign(faceIndices, faceIndices + 6);
            path = "$GAME_RESOURCES$/harvestClientData/gfx/";
            path.append(core::CString<char>(faceTextures[f]));
            path.append(core::CString<char>(".jpg"));
            skybox[f].Material.Texture1 = driver->getTexture(path.c_str());
            skybox[f].Material.Lighting = false;
            skybox[f].Material.ZBuffer = false;
            skybox[f].Material.ZWriteEnable = false;
            skybox[f].Position = camera;
            skybox[f].Angle = 0.0f;
        }
    }

    // the sun: an additive billboard, lighting and z-buffer off
    sun.Position = CVector3d<float>(-195.0f, 12.0f, 20.0f);
    sun.Angle = 0.0f;
    const float half = 9.0f;
    sun.Vertices.push_back(video::S3DVertex(-half, -half, 0, 0, 0, -1, SColor(0xffffffff), 0, 1));
    sun.Vertices.push_back(video::S3DVertex(-half, half, 0, 0, 0, -1, SColor(0xffffffff), 0, 0));
    sun.Vertices.push_back(video::S3DVertex(half, half, 0, 0, 0, -1, SColor(0xffffffff), 1, 0));
    sun.Vertices.push_back(video::S3DVertex(half, -half, 0, 0, 0, -1, SColor(0xffffffff), 1, 1));
    unsigned short sunIndices[6] = {0, 1, 2, 0, 2, 3};
    sun.Indices.assign(sunIndices, sunIndices + 6);
    sun.Material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
    sun.Material.Lighting = false;
    sun.Material.ZBuffer = false;
    sun.Material.Texture1 = driver->getTexture("$GAME_RESOURCES$/harvestClientData/gfx/particlewhite.jpg");


    SMenuScene menuScene;
    if (!menuScene.build(driver, fileSystem, shaderLevel))
        fprintf(stderr, "cannot load the menu's meshes\n");

    // Scene 0 is the 2D one, 1 the 3D one, 2 the menu; space switches between them.
    const int sceneCount = 3;
    int scene = sceneArgument;
    bool running = true;
    for (int frame = 0; running; ++frame)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE)
                scene = (scene + 1) % sceneCount;
        }
        if (screenshotDirectory && sceneArgument < 0)
            scene = frame / frames;
        else if (sceneArgument < 0 && frame == 0)
            scene = 0;

        const float t = frame * 0.01f;
        const CDimension2d<int> screen = driver->getScreenSize();

        driver->beginScene(true, true, SColor(255, 0, 0, 0));

        if (scene == 2)
        {
            // the menu's camera targets: planet selection, the opening view, after picking Poseidon
            // (screenshots show the first)
            static const CVector3d<float> views[3][2] = {
                {CVector3d<float>(-200.0f, 40.0f, -100.0f), CVector3d<float>(-200.0f, 0.0f, 0.0f)},
                {CVector3d<float>(-200.0f, -20.0f, -150.0f), CVector3d<float>(-350.0f, 0.0f, -70.0f)},
                {CVector3d<float>(-187.0f, 20.0f, 70.0f), CVector3d<float>(-187.0f, 20.0f, 80.0f)},
            };
            const int view = screenshotDirectory ? 0 : (frame / 120) % 3;
            menuScene.Camera->setPosition(views[view][0]);
            menuScene.Camera->setTarget(views[view][1]);
            menuScene.SceneManager->drawAll();
            char text[64];
            snprintf(text, sizeof(text), "main menu scene, shader level %d", shaderLevel);
            smallFont.draw(text, CPosition2d<int>(16, 12), SColor(0xffffd040));
        }
        else if (scene == 1)
        {
            for (int i = 0; i < 3; ++i)
                planets[i].Angle = t * (i + 5) * 0.1f;
            atmosphere.Angle = planets[0].Angle;

            // in the scene manager's order: camera, light, opaque nodes, transparent nodes
            driver->setTransform(video::ETS_PROJECTION,
                perspectiveLH(3.14159265f / 2.5f, (float)screen.Height / screen.Width, 1.0f, 3000.0f));
            driver->setTransform(video::ETS_VIEW, lookAtLH(camera, CVector3d<float>(-225.0f, 0.0f, 0.0f),
                CVector3d<float>(0.0f, 1.0f, 0.0f)));
            driver->deleteAllDynamicLights();
            driver->setAmbientLight(video::SColorf(SColor(255, 32, 32, 32)));
            video::SLight light;
            light.AmbientColor = video::SColorf(0, 0, 0, 1);
            light.DiffuseColor = video::SColorf(117 / 255.0f, 117 / 255.0f, 117 / 255.0f, 1.0f);
            light.SpecularColor = video::SColorf(1, 1, 1, 1);
            light.Position = CVector3d<float>(800.0f, 0.0f, 0.0f);
            light.Radius = 7500.0f;
            light.CastShadows = false;
            light.Directional = false;
            driver->addDynamicLight(light);

            for (int f = 0; f < 6; ++f)
                drawNode(driver, skybox[f]);
            for (int i = 0; i < 3; ++i)
                drawNode(driver, planets[i]);
            if (atmoMaterial != -1)
                drawNode(driver, atmosphere);
            drawNode(driver, sun);

            // a 2D overlay over the 3D frame, as the main menu draws its GUI
            font.draw("high: scatterGroundCG + scatterAtmoCG", CPosition2d<int>(16, screen.Height - 120),
                SColor(0xffffffff));
            font.draw("low: scatterGroundSCG", CPosition2d<int>(16, screen.Height - 85), SColor(0xffffffff));
            font.draw("none: EMT_SOLID", CPosition2d<int>(16, screen.Height - 50), SColor(0xffffffff));
            driver->draw2DRectangle(SColor(0x60000000), CRect<int>(0, 0, screen.Width, 40), 0);
            smallFont.draw("main menu planets: Hephaestus, Poseidon, Ares; additive sun billboard",
                CPosition2d<int>(16, 12), SColor(0xffffd040));
        }
        else
        {
            // the sprites of harvestMenu.dat in a grid, at their own size, fitted into cells
            int x = 8, y = 8, rowHeight = 0;
            for (unsigned int i = 0; i < sprites.size() && y < screen.Height - 300; ++i)
            {
                if (!sprites[i])
                    continue;
                CDimension2d<int> size = sprites[i]->getFrameOriginalSize(0);
                float scale = 1.0f;
                if (size.Width > 200 || size.Height > 80)
                    scale = size.Width * 80 > size.Height * 200 ? 200.0f / size.Width : 80.0f / size.Height;
                int w = (int)(size.Width * scale), h = (int)(size.Height * scale);
                if (x + w > screen.Width - 8)
                {
                    x = 8;
                    y += rowHeight + 6;
                    rowHeight = 0;
                }
                if (scale == 1.0f)
                    sprites[i]->draw(CPosition2d<int>(x, y), 0, SColor(0xffffffff));
                else
                    sprites[i]->drawScaled(CPosition2d<float>((float)x, (float)y), scale, SColor(0xffffffff));
                x += w + 6;
                if (h > rowHeight)
                    rowHeight = h;
            }

            // every draw2DImage variant on one sprite's frame (the planet emblem of the menu)
            video::ISpriteAnimationState* sample = sprites.size() > 2 ? sprites[2] : 0;
            video::ITexture* texture = sample ? sample->getFrameTexture(0) : 0;
            const int baseY = screen.Height - 280;
            if (texture)
            {
                const CRect<int> source = sample->getFrameTexturePosition(0);
                const int w = source.getWidth(), h = source.getHeight();
                int px = 8;
                // colour, alpha channel
                driver->draw2DImage(texture, CPosition2d<int>(px, baseY), source, 0, SColor(0xffffffff), true);
                px += w + 10;
                // colour array (c[0], c[3], c[2], c[1]), +0.5 px
                SColor corners[4] = {SColor(0xffff0000), SColor(0xff00ff00), SColor(0xff0000ff), SColor(0xffffffff)};
                driver->draw2DImage(texture, CPosition2d<int>(px, baseY), source, 0, corners, true);
                px += w + 10;
                // integer corners, axis aligned: linear filtering, corner colours
                SColor cornerColors[4] = {SColor(0xffffffff), SColor(0x80ffffff), SColor(0xffffff00),
                    SColor(0x40ffffff)};
                driver->draw2DImage(texture, CPosition2d<int>(px, baseY), CPosition2d<int>(px + 2 * w, baseY),
                    CPosition2d<int>(px, baseY + 2 * h), CPosition2d<int>(px + 2 * w, baseY + 2 * h), source,
                    cornerColors, true);
                px += 2 * w + 10;
                // integer corners, sheared: nearest filtering
                driver->draw2DImage(texture, CPosition2d<int>(px + 20, baseY), CPosition2d<int>(px + 2 * w + 20, baseY),
                    CPosition2d<int>(px, baseY + 2 * h), CPosition2d<int>(px + 2 * w, baseY + 2 * h), source, 0, true);
                px += 2 * w + 30;
                // scaled (float corners, linear)
                driver->drawScaled2DImage(texture, CPosition2d<float>((float)px + 0.5f, (float)baseY), source, 2.0f,
                    SColor(0xffffffff), true);
                px += 2 * w + 10;
                // without the alpha channel, clipped to a rectangle
                CRect<int> clip(px + 8, baseY + 8, px + w - 8, baseY + h - 8);
                driver->draw2DImage(texture, CPosition2d<int>(px, baseY), source, &clip, SColor(0xffffffff), false);
                px += w + 10;
                // tinted, translucent
                driver->draw2DImage(texture, CPosition2d<int>(px, baseY), source, 0, SColor(0x8040ff40), true);
                px += w + 10;
                // rotated
                sample->drawRotated(CPosition2d<float>((float)px + w, (float)baseY + h), t, 1.0f, SColor(0xffffffff));
            }

            // the GUI's built-in font, a 4-bit BMP, as CGUIEnvironment loads it
            if (builtInFont)
                driver->draw2DImage(builtInFont, CPosition2d<int>(screen.Width - 132, screen.Height - 132));

            // rectangles and lines
            const int lineY = screen.Height - 110;
            driver->draw2DRectangle(SColor(0xffc04020), CRect<int>(8, lineY, 108, lineY + 40), 0);
            driver->draw2DRectangle(SColor(0x8020a0ff), CRect<int>(60, lineY + 20, 160, lineY + 60), 0);
            CRect<int> rectClip(200, lineY, 260, lineY + 60);
            driver->draw2DRectangle(SColor(0xff40ff40), CRect<int>(180, lineY + 10, 280, lineY + 50), &rectClip);
            for (int i = 0; i < 8; ++i)
            {
                driver->draw2DLine(CPosition2d<int>(300 + i * 10, lineY), CPosition2d<int>(360 + i * 10, lineY + 60),
                    SColor(i & 1 ? 0xffffffff : 0x80ffff00));
                driver->draw2DLineFloat(CPosition2d<float>(400.5f, lineY + i * 8.0f),
                    CPosition2d<float>(520.5f, lineY + i * 8.0f + 4.0f), SColor(0xff00ffff));
            }

            // text, and a scissor rectangle that cuts a line of it
            font.draw("Harvest: Massive Encounter", CPosition2d<int>(560, lineY - 10), SColor(0xffffffff));
            CRect<int> scissor(560, 0, 760, screen.Height);
            driver->setScissorRect(&scissor);
            smallFont.draw("OpenGL ES 3.0 / GL 3.3 renderer, scissored text", CPosition2d<int>(560, lineY + 40),
                SColor(0xffffd040));
            driver->setScissorRect(0);
        }

        if (screenshotDirectory && (frame + 1) % frames == 0)
        {
            // the game passes a directory ending in a slash
            core::CString<char> directory = screenshotDirectory;
            if (directory.size() > 1 && directory[directory.size() - 2] != '/')
                directory.append('/');
            const char* names[sceneCount] = {"video-test-2d-", "video-test-3d-", "video-test-menu-"};
            driver->saveJpegScreenshot(directory.c_str(), names[scene]);
            if (sceneArgument >= 0 || scene == sceneCount - 1)
                running = false;
        }
        driver->endScene();
    }

    driver->drop();
    fileSystem->drop();
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
