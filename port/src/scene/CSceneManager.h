// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CSCENEMANAGER_H
#define PORT_SCENE_CSCENEMANAGER_H

#include <map>
#include <string>
#include <vector>
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "scene/SceneMath.h"

namespace ox {
namespace gui { class ICursorControl; }
namespace io { class IFileSystem; }
} // end namespace ox

namespace daisy {
namespace scene {

//! The scene manager the main menu needs: OBJ meshes, animated mesh, camera, light, billboard
//! and skybox nodes, the rotation animator, bounding-box picking and Irrlicht 0.7's render passes.
//! It is also the root node of the scene graph. Everything else in ISceneManager returns null or
//! does nothing; the game never calls it.
class CSceneManager : public ox::scene::ISceneManager, public ox::scene::ISceneNode
{
public:
    CSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs, ox::gui::ICursorControl* cursorControl);
    virtual ~CSceneManager();

    // The parts of the interface the game uses.
    virtual ox::scene::IAnimatedMesh* getMesh(const char* filename);
    virtual ox::video::IVideoDriver* getVideoDriver();
    virtual ox::scene::IAnimatedMeshSceneNode* addAnimatedMeshSceneNode(ox::scene::IAnimatedMesh* mesh,
        ox::scene::ISceneNode* parent, int id, const vector3df& position, const vector3df& rotation,
        const vector3df& scale);
    virtual ox::scene::ICameraSceneNode* addCameraSceneNode(ox::scene::ISceneNode* parent, const vector3df& position,
        const vector3df& lookat, int id);
    virtual ox::scene::ILightSceneNode* addLightSceneNode(ox::scene::ISceneNode* parent, const vector3df& position,
        ox::video::SColorf color, float radius, int id);
    virtual ox::scene::IBillboardSceneNode* addBillboardSceneNode(ox::scene::ISceneNode* parent,
        const ox::core::CDimension2d<float>& size, const vector3df& position, int id);
    virtual ox::scene::ISceneNode* addSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom,
        ox::video::ITexture* left, ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back,
        ox::scene::ISceneNode* parent, int id);
    virtual ox::scene::ISceneNode* getRootSceneNode();
    virtual ox::scene::ISceneNode* getSceneNodeFromId(int id, ox::scene::ISceneNode* start);
    virtual ox::scene::ICameraSceneNode* getActiveCamera();
    virtual void setActiveCamera(ox::scene::ICameraSceneNode* camera);
    virtual void setShadowColor(ox::video::SColor color);
    virtual ox::video::SColor getShadowColor() const;
    virtual void registerNodeForRendering(ox::scene::ISceneNode* node, ox::scene::E_SCENE_NODE_RENDER_TIME time);
    virtual void drawAll();
    virtual ox::scene::ISceneNodeAnimator* createRotationAnimator(const vector3df& rotationPerSecond);
    virtual ox::scene::ISceneCollisionManager* getSceneCollisionManager();
    virtual void addToDeletionQueue(ox::scene::ISceneNode* node);
    virtual bool postEventFromUser(ox::event::SEvent event);
    virtual void clear();
    virtual void removeAllMeshes();
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;
    virtual void removeAll();

    // Not reached by the game.
    virtual ox::scene::ISceneNode* addTestSceneNode(float size, ox::scene::ISceneNode* parent, int id,
        const vector3df& position, const vector3df& rotation, const vector3df& scale);
    virtual ox::scene::ISceneNode* addMeshSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent, int id,
        const vector3df& position, const vector3df& rotation, const vector3df& scale);
    virtual ox::scene::ISceneNode* addWaterSurfaceSceneNode(ox::scene::IMesh* mesh, float waveHeight, float waveSpeed,
        float waveLength, ox::scene::ISceneNode* parent, int id, const vector3df& position, const vector3df& rotation,
        const vector3df& scale);
    virtual ox::scene::ISceneNode* addBspTreeSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent, int id);
    virtual ox::scene::ISceneNode* addOctTreeSceneNode(ox::scene::IAnimatedMesh* mesh, ox::scene::ISceneNode* parent,
        int id, int minimalPolysPerNode);
    virtual ox::scene::ISceneNode* addOctTreeSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent, int id,
        int minimalPolysPerNode);
    virtual ox::scene::ICameraSceneNode* addCameraSceneNodeMaya(ox::scene::ISceneNode* parent, float rotateSpeed,
        float zoomSpeed, float translationSpeed, int id);
    virtual ox::scene::ICameraSceneNode* addCameraSceneNodeFPS(ox::scene::ISceneNode* parent, float rotateSpeed,
        float moveSpeed, int id, ox::SKeyMap* keyMapArray, int keyMapSize);
    virtual ox::scene::IParticleSystemSceneNode* addParticleSystemSceneNode(bool withDefaultEmitter,
        ox::scene::ISceneNode* parent, int id, const vector3df& position, const vector3df& rotation,
        const vector3df& scale);
    virtual ox::scene::ITerrainSceneNode* addTerrainSceneNode(ox::scene::ISceneNode* parent, int id,
        ox::video::IImage* texture, ox::video::IImage* heightmap, ox::video::ITexture* detailmap,
        const ox::core::CDimension2d<float>& stretchSize, float maxHeight,
        const ox::core::CDimension2d<int>& defaultVertexBlockSize);
    virtual ox::scene::ISceneNode* addEmptySceneNode(ox::scene::ISceneNode* parent, int id);
    virtual ox::scene::IDummyTransformationSceneNode* addDummyTransformationSceneNode(ox::scene::ISceneNode* parent,
        int id);
    virtual ox::scene::ITextSceneNode* addTextSceneNode(ox::gui::IGUIFont* font, const wchar_t* text,
        ox::video::SColor color, ox::scene::ISceneNode* parent, const vector3df& position, int id);
    virtual ox::scene::IAnimatedMesh* addHillPlaneMesh(const char* name, const ox::core::CDimension2d<float>& tileSize,
        const ox::core::CDimension2d<int>& tileCount, ox::video::SMaterial* material, float hillHeight,
        const ox::core::CDimension2d<float>& countHills, const ox::core::CDimension2d<float>& textureRepeatCount);
    virtual ox::scene::IAnimatedMesh* addTerrainMesh(const char* meshname, ox::video::IImage* texture,
        ox::video::IImage* heightmap, const ox::core::CDimension2d<float>& stretchSize, float maxHeight,
        const ox::core::CDimension2d<int>& defaultVertexBlockSize);
    virtual ox::scene::ISceneNodeAnimator* createFlyCircleAnimator(const vector3df& center, float radius, float speed);
    virtual ox::scene::ISceneNodeAnimator* createFlyStraightAnimator(const vector3df& startPoint,
        const vector3df& endPoint, unsigned int timeForWay, bool loop);
    virtual ox::scene::ISceneNodeAnimator* createTextureAnimator(const ox::TArray<ox::video::ITexture*>& textures,
        int timePerFrame, bool loop);
    virtual ox::scene::ISceneNodeAnimator* createDeleteAnimator(unsigned int timeMs);
    virtual ox::scene::ISceneNodeAnimatorCollisionResponse* createCollisionResponseAnimator(
        ox::scene::ITriangleSelector* world, ox::scene::ISceneNode* sceneNode, const vector3df& ellipsoidRadius,
        const vector3df& gravityPerSecond, float accelerationPerSecond, const vector3df& ellipsoidTranslation,
        float slidingValue);
    virtual ox::scene::ISceneNodeAnimator* createFollowSplineAnimator(int startTime,
        const ox::TArray<vector3df>& points, float speed, float tightness);
    virtual ox::scene::ITriangleSelector* createTriangleSelector(ox::scene::IMesh* mesh, ox::scene::ISceneNode* node);
    virtual ox::scene::ITriangleSelector* createTriangleSelectorFromBoundingBox(ox::scene::ISceneNode* node);
    virtual ox::scene::ITriangleSelector* createOctTreeTriangleSelector(ox::scene::IMesh* mesh,
        ox::scene::ISceneNode* node, int minimalPolysPerNode);
    virtual ox::scene::IMetaTriangleSelector* createMetaTriangleSelector();
    virtual void addExternalMeshLoader(ox::scene::IMeshLoader* externalLoader);
    virtual ox::scene::IMeshManipulator* getMeshManipulator();

private:
    //! True if the node's transformed box misses the active camera's frustum box.
    bool isCulled(ox::scene::ISceneNode* node);
    void clearDeletionList();

    //! An opaque node with the first texture of its first material, the sort key.
    struct DefaultNodeEntry
    {
        explicit DefaultNodeEntry(ox::scene::ISceneNode* n);

        bool operator<(const DefaultNodeEntry& other) const
        {
            return textureValue < other.textureValue;
        }

        ox::scene::ISceneNode* node;
        ox::video::ITexture* textureValue;
    };

    //! A transparent node and its distance from the camera, the sort key (nearest first).
    struct TransparentNodeEntry
    {
        TransparentNodeEntry(ox::scene::ISceneNode* n, const vector3df& camera);

        bool operator<(const TransparentNodeEntry& other) const
        {
            return distance < other.distance;
        }

        ox::scene::ISceneNode* node;
        float distance;
    };

    //! Loaded meshes by lowercased name.
    std::map<std::string, ox::scene::IAnimatedMesh*> Meshes;

    ox::video::IVideoDriver* Driver;
    ox::io::IFileSystem* FileSystem;
    ox::gui::ICursorControl* CursorControl;
    ox::scene::ISceneCollisionManager* CollisionManager;

    std::vector<ox::scene::ISceneNode*> LightAndCameraList;
    std::vector<ox::scene::ISceneNode*> ShadowNodeList;
    std::vector<ox::scene::ISceneNode*> SkyBoxList;
    std::vector<DefaultNodeEntry> DefaultNodeList;
    std::vector<TransparentNodeEntry> TransparentNodeList;
    std::vector<ox::scene::ISceneNode*> DeletionList;

    ox::scene::ICameraSceneNode* ActiveCamera;
    //! The active camera's absolute position at the start of drawAll.
    vector3df camTransPos;
    ox::video::SColor ShadowColor;
};

} // end namespace scene
} // end namespace daisy

#endif
