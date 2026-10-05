// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::scene::CSceneManager; return types are provisional where no
// recovered caller uses them.

#ifndef OX_SCENE_ISCENEMANAGER_H
#define OX_SCENE_ISCENEMANAGER_H

#include "../IUnknown.h"
#include "../TArray.h"
#include "../core/CDimension2d.h"
#include "../core/CVector3d.h"
#include "../event/IEventReceiver.h"
#include "../video/SColor.h"
#include "../video/SLight.h"

namespace ox {
struct SKeyMap;
namespace core { template <class T> class CAabbox3d; }
namespace gui { class IGUIFont; }
namespace video {
class IImage;
class ITexture;
class IVideoDriver;
struct SMaterial;
} // end namespace video
namespace scene {

class IAnimatedMesh;
class IAnimatedMeshSceneNode;
class IBillboardSceneNode;
class ICameraSceneNode;
class IDummyTransformationSceneNode;
class ILightSceneNode;
class IMesh;
class IMeshLoader;
class IMeshManipulator;
class IMetaTriangleSelector;
class IParticleSystemSceneNode;
class ISceneCollisionManager;
class ISceneNode;
class ISceneNodeAnimator;
class ISceneNodeAnimatorCollisionResponse;
class ITerrainSceneNode;
class ITextSceneNode;
class ITriangleSelector;

//! When a registered node is rendered; the enumerators are not recovered.
enum E_SCENE_NODE_RENDER_TIME
{
};

//! Owns the scene graph and creates scene nodes, animators and triangle selectors.
class ISceneManager : public IUnknown
{
public:
    virtual IAnimatedMesh* getMesh(const char* filename) = 0;
    virtual video::IVideoDriver* getVideoDriver() = 0;
    virtual ISceneNode* addTestSceneNode(float size, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale) = 0;
    virtual IAnimatedMeshSceneNode* addAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent = 0,
        int id = -1, const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& rotation = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& scale = core::CVector3d<float>(1.0f, 1.0f, 1.0f)) = 0;
    virtual ISceneNode* addMeshSceneNode(IMesh* mesh, ISceneNode* parent, int id,
        const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale) = 0;
    virtual ISceneNode* addWaterSurfaceSceneNode(IMesh* mesh, float waveHeight, float waveSpeed,
        float waveLength, ISceneNode* parent, int id, const core::CVector3d<float>& position,
        const core::CVector3d<float>& rotation, const core::CVector3d<float>& scale) = 0;
    virtual ISceneNode* addBspTreeSceneNode(IMesh* mesh, ISceneNode* parent, int id) = 0;
    virtual ISceneNode* addOctTreeSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, int id,
        int minimalPolysPerNode) = 0;
    virtual ISceneNode* addOctTreeSceneNode(IMesh* mesh, ISceneNode* parent, int id,
        int minimalPolysPerNode) = 0;
    virtual ICameraSceneNode* addCameraSceneNode(ISceneNode* parent = 0,
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& lookat = core::CVector3d<float>(0, 0, 100), int id = -1) = 0;
    virtual ICameraSceneNode* addCameraSceneNodeMaya(ISceneNode* parent, float rotateSpeed, float zoomSpeed,
        float translationSpeed, int id) = 0;
    virtual ICameraSceneNode* addCameraSceneNodeFPS(ISceneNode* parent, float rotateSpeed, float moveSpeed,
        int id, SKeyMap* keyMapArray, int keyMapSize) = 0;
    virtual ILightSceneNode* addLightSceneNode(ISceneNode* parent = 0,
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0),
        video::SColorf color = video::SColorf(1.0f, 1.0f, 1.0f), float radius = 100.0f, int id = -1) = 0;
    virtual IBillboardSceneNode* addBillboardSceneNode(ISceneNode* parent = 0,
        const core::CDimension2d<float>& size = core::CDimension2d<float>(10.0f, 10.0f),
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0), int id = -1) = 0;
    virtual ISceneNode* addSkyBoxSceneNode(video::ITexture* top, video::ITexture* bottom, video::ITexture* left,
        video::ITexture* right, video::ITexture* front, video::ITexture* back, ISceneNode* parent = 0,
        int id = -1) = 0;
    virtual IParticleSystemSceneNode* addParticleSystemSceneNode(bool withDefaultEmitter, ISceneNode* parent,
        int id, const core::CVector3d<float>& position, const core::CVector3d<float>& rotation,
        const core::CVector3d<float>& scale) = 0;
    virtual ITerrainSceneNode* addTerrainSceneNode(ISceneNode* parent, int id, video::IImage* texture,
        video::IImage* heightmap, video::ITexture* detailmap, const core::CDimension2d<float>& stretchSize,
        float maxHeight, const core::CDimension2d<int>& defaultVertexBlockSize) = 0;
    virtual ISceneNode* addEmptySceneNode(ISceneNode* parent, int id) = 0;
    virtual IDummyTransformationSceneNode* addDummyTransformationSceneNode(ISceneNode* parent, int id) = 0;
    virtual ITextSceneNode* addTextSceneNode(gui::IGUIFont* font, const wchar_t* text, video::SColor color,
        ISceneNode* parent, const core::CVector3d<float>& position, int id) = 0;
    virtual IAnimatedMesh* addHillPlaneMesh(const char* name, const core::CDimension2d<float>& tileSize,
        const core::CDimension2d<int>& tileCount, video::SMaterial* material, float hillHeight,
        const core::CDimension2d<float>& countHills, const core::CDimension2d<float>& textureRepeatCount) = 0;
    virtual IAnimatedMesh* addTerrainMesh(const char* meshname, video::IImage* texture, video::IImage* heightmap,
        const core::CDimension2d<float>& stretchSize, float maxHeight,
        const core::CDimension2d<int>& defaultVertexBlockSize) = 0;
    virtual ISceneNode* getRootSceneNode() = 0;
    virtual ISceneNode* getSceneNodeFromId(int id, ISceneNode* start = 0) = 0;
    virtual ICameraSceneNode* getActiveCamera() = 0;
    virtual void setActiveCamera(ICameraSceneNode* camera) = 0;
    virtual void setShadowColor(video::SColor color) = 0;
    virtual video::SColor getShadowColor() const = 0;
    virtual void registerNodeForRendering(ISceneNode* node, E_SCENE_NODE_RENDER_TIME time) = 0;
    virtual void drawAll() = 0;
    virtual ISceneNodeAnimator* createRotationAnimator(const core::CVector3d<float>& rotationPerSecond) = 0;
    virtual ISceneNodeAnimator* createFlyCircleAnimator(const core::CVector3d<float>& center, float radius,
        float speed) = 0;
    virtual ISceneNodeAnimator* createFlyStraightAnimator(const core::CVector3d<float>& startPoint,
        const core::CVector3d<float>& endPoint, unsigned int timeForWay, bool loop) = 0;
    virtual ISceneNodeAnimator* createTextureAnimator(const TArray<video::ITexture*>& textures,
        int timePerFrame, bool loop) = 0;
    virtual ISceneNodeAnimator* createDeleteAnimator(unsigned int timeMs) = 0;
    virtual ISceneNodeAnimatorCollisionResponse* createCollisionResponseAnimator(ITriangleSelector* world,
        ISceneNode* sceneNode, const core::CVector3d<float>& ellipsoidRadius,
        const core::CVector3d<float>& gravityPerSecond, float accelerationPerSecond,
        const core::CVector3d<float>& ellipsoidTranslation, float slidingValue) = 0;
    virtual ISceneNodeAnimator* createFollowSplineAnimator(int startTime,
        const TArray<core::CVector3d<float> >& points, float speed, float tightness) = 0;
    virtual ITriangleSelector* createTriangleSelector(IMesh* mesh, ISceneNode* node) = 0;
    virtual ITriangleSelector* createTriangleSelectorFromBoundingBox(ISceneNode* node) = 0;
    virtual ITriangleSelector* createOctTreeTriangleSelector(IMesh* mesh, ISceneNode* node,
        int minimalPolysPerNode) = 0;
    virtual IMetaTriangleSelector* createMetaTriangleSelector() = 0;
    virtual void addExternalMeshLoader(IMeshLoader* externalLoader) = 0;
    virtual ISceneCollisionManager* getSceneCollisionManager() = 0;
    virtual IMeshManipulator* getMeshManipulator() = 0;
    virtual void addToDeletionQueue(ISceneNode* node) = 0;
    virtual bool postEventFromUser(event::SEvent event) = 0;
    virtual void clear() = 0;
    virtual void removeAllMeshes() = 0;
    virtual void render() = 0;
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
    virtual void removeAll() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
