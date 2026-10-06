// Temporary: weak definitions of the seams other parts of the port implement (the renderer in
// port/src/video/, the audio backend in port/src/audio/, the menu scene manager in port/src/scene/),
// so the game links and runs before they land. A strong definition anywhere replaces the one here;
// delete this file once all three exist.

#include <SDL3/SDL.h>
#include "device/NullDrivers.h"
#include "platform/Seams.h"
#include "ox/core/CAabbox3d.h"
#include "ox/gui/ICursorControl.h"
#include "ox/scene/ISceneManager.h"

#define PORT_WEAK __attribute__((weak))

namespace port {

PORT_WEAK ox::video::IVideoDriver* createVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
    const ox::core::CDimension2d<int>& screenSize)
{
    SDL_Log("no renderer linked: using the null video driver");
    return createNullVideoDriver(device, fileSystem, screenSize);
}

PORT_WEAK ox::audio::IAudioDriver* createAudioDriver(ox::io::IFileSystem* fileSystem)
{
    SDL_Log("no audio backend linked: sound is off");
    return createNullAudioDriver();
}

} // end namespace port

namespace daisy {
namespace scene {

namespace {

using namespace ox;
using namespace ox::scene;

//! Creates nothing: CGameState only needs a scene manager to exist, and the main menu, the only user
//! of the scene, stops loading its planets when getMesh fails.
class CNullSceneManager : public ISceneManager
{
public:
    CNullSceneManager(video::IVideoDriver* driver) : Driver(driver) {}

    virtual IAnimatedMesh* getMesh(const char* filename) { return 0; }
    virtual video::IVideoDriver* getVideoDriver() { return Driver; }
    virtual ISceneNode* addTestSceneNode(float size, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale)
    {
        return 0;
    }
    virtual IAnimatedMeshSceneNode* addAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale)
    {
        return 0;
    }
    virtual ISceneNode* addMeshSceneNode(IMesh* mesh, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale)
    {
        return 0;
    }
    virtual ISceneNode* addWaterSurfaceSceneNode(IMesh* mesh, float waveHeight, float waveSpeed, float waveLength,
        ISceneNode* parent, int id, const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale)
    {
        return 0;
    }
    virtual ISceneNode* addBspTreeSceneNode(IMesh* mesh, ISceneNode* parent, int id) { return 0; }
    virtual ISceneNode* addOctTreeSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, int id, int minimalPolysPerNode)
    {
        return 0;
    }
    virtual ISceneNode* addOctTreeSceneNode(IMesh* mesh, ISceneNode* parent, int id, int minimalPolysPerNode)
    {
        return 0;
    }
    virtual ICameraSceneNode* addCameraSceneNode(ISceneNode* parent, const core::CVector3d<float>& position,
        const core::CVector3d<float>& lookat, int id)
    {
        return 0;
    }
    virtual ICameraSceneNode* addCameraSceneNodeMaya(ISceneNode* parent, float rotateSpeed, float zoomSpeed,
        float translationSpeed, int id)
    {
        return 0;
    }
    virtual ICameraSceneNode* addCameraSceneNodeFPS(ISceneNode* parent, float rotateSpeed, float moveSpeed, int id,
        SKeyMap* keyMapArray, int keyMapSize)
    {
        return 0;
    }
    virtual ILightSceneNode* addLightSceneNode(ISceneNode* parent, const core::CVector3d<float>& position,
        video::SColorf color, float radius, int id)
    {
        return 0;
    }
    virtual IBillboardSceneNode* addBillboardSceneNode(ISceneNode* parent, const core::CDimension2d<float>& size,
        const core::CVector3d<float>& position, int id)
    {
        return 0;
    }
    virtual ISceneNode* addSkyBoxSceneNode(video::ITexture* top, video::ITexture* bottom, video::ITexture* left,
        video::ITexture* right, video::ITexture* front, video::ITexture* back, ISceneNode* parent, int id)
    {
        return 0;
    }
    virtual IParticleSystemSceneNode* addParticleSystemSceneNode(bool withDefaultEmitter, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale)
    {
        return 0;
    }
    virtual ITerrainSceneNode* addTerrainSceneNode(ISceneNode* parent, int id, video::IImage* texture,
        video::IImage* heightmap, video::ITexture* detailmap, const core::CDimension2d<float>& stretchSize,
        float maxHeight, const core::CDimension2d<int>& defaultVertexBlockSize)
    {
        return 0;
    }
    virtual ISceneNode* addEmptySceneNode(ISceneNode* parent, int id) { return 0; }
    virtual IDummyTransformationSceneNode* addDummyTransformationSceneNode(ISceneNode* parent, int id) { return 0; }
    virtual ITextSceneNode* addTextSceneNode(gui::IGUIFont* font, const wchar_t* text, video::SColor color,
        ISceneNode* parent, const core::CVector3d<float>& position, int id)
    {
        return 0;
    }
    virtual IAnimatedMesh* addHillPlaneMesh(const char* name, const core::CDimension2d<float>& tileSize,
        const core::CDimension2d<int>& tileCount, video::SMaterial* material, float hillHeight,
        const core::CDimension2d<float>& countHills, const core::CDimension2d<float>& textureRepeatCount)
    {
        return 0;
    }
    virtual IAnimatedMesh* addTerrainMesh(const char* meshname, video::IImage* texture, video::IImage* heightmap,
        const core::CDimension2d<float>& stretchSize, float maxHeight,
        const core::CDimension2d<int>& defaultVertexBlockSize)
    {
        return 0;
    }
    virtual ISceneNode* getRootSceneNode() { return 0; }
    virtual ISceneNode* getSceneNodeFromId(int id, ISceneNode* start) { return 0; }
    virtual ICameraSceneNode* getActiveCamera() { return 0; }
    virtual void setActiveCamera(ICameraSceneNode* camera) {}
    virtual void setShadowColor(video::SColor color) {}
    virtual video::SColor getShadowColor() const { return video::SColor(0); }
    virtual void registerNodeForRendering(ISceneNode* node, E_SCENE_NODE_RENDER_TIME time) {}
    virtual void drawAll() {}
    virtual ISceneNodeAnimator* createRotationAnimator(const core::CVector3d<float>& rotationPerSecond) { return 0; }
    virtual ISceneNodeAnimator* createFlyCircleAnimator(const core::CVector3d<float>& center, float radius,
        float speed)
    {
        return 0;
    }
    virtual ISceneNodeAnimator* createFlyStraightAnimator(const core::CVector3d<float>& startPoint,
        const core::CVector3d<float>& endPoint, unsigned int timeForWay, bool loop)
    {
        return 0;
    }
    virtual ISceneNodeAnimator* createTextureAnimator(const TArray<video::ITexture*>& textures, int timePerFrame,
        bool loop)
    {
        return 0;
    }
    virtual ISceneNodeAnimator* createDeleteAnimator(unsigned int timeMs) { return 0; }
    virtual ISceneNodeAnimatorCollisionResponse* createCollisionResponseAnimator(ITriangleSelector* world,
        ISceneNode* sceneNode, const core::CVector3d<float>& ellipsoidRadius,
        const core::CVector3d<float>& gravityPerSecond, float accelerationPerSecond,
        const core::CVector3d<float>& ellipsoidTranslation, float slidingValue)
    {
        return 0;
    }
    virtual ISceneNodeAnimator* createFollowSplineAnimator(int startTime, const TArray<core::CVector3d<float> >& points,
        float speed, float tightness)
    {
        return 0;
    }
    virtual ITriangleSelector* createTriangleSelector(IMesh* mesh, ISceneNode* node) { return 0; }
    virtual ITriangleSelector* createTriangleSelectorFromBoundingBox(ISceneNode* node) { return 0; }
    virtual ITriangleSelector* createOctTreeTriangleSelector(IMesh* mesh, ISceneNode* node, int minimalPolysPerNode)
    {
        return 0;
    }
    virtual IMetaTriangleSelector* createMetaTriangleSelector() { return 0; }
    virtual void addExternalMeshLoader(IMeshLoader* externalLoader) {}
    virtual ISceneCollisionManager* getSceneCollisionManager() { return 0; }
    virtual IMeshManipulator* getMeshManipulator() { return 0; }
    virtual void addToDeletionQueue(ISceneNode* node) {}
    virtual bool postEventFromUser(event::SEvent event) { return false; }
    virtual void clear() {}
    virtual void removeAllMeshes() {}
    virtual void render() {}
    virtual const core::CAabbox3d<float>& getBoundingBox() const { return Box; }
    virtual void removeAll() {}

private:
    video::IVideoDriver* Driver;
    core::CAabbox3d<float> Box;
};

} // end namespace

PORT_WEAK ox::scene::ISceneManager* createSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
    ox::gui::ICursorControl* cursorControl)
{
    SDL_Log("no scene manager linked: the main menu has no planets");
    return new CNullSceneManager(driver);
}

} // end namespace scene
} // end namespace daisy
