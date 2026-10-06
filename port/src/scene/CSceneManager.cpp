// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneManager.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Checked against the Mac 1.18 build (0x110bba-0x112f10). daisy's scene manager is Irrlicht 0.7's
// with std::vector render lists sorted by std::sort (the opaque list by first texture, the
// transparent list nearest first), clear() as removeAll(), and removeAllMeshes() added. Meshes are
// cached under their lowercased name and stay cached across clear(), as in the original.
//
// Of the mesh loaders only OBJ is ported; the menu loads nothing else.

#include "scene/CSceneManager.h"
#include <algorithm>
#include <string.h>
#include "daisy/os.h"
#include "ox/core/CString.h"
#include "ox/core/CStringFunctions.h"
#include "ox/event/ILogger.h"
#include "ox/gui/ICursorControl.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/IVideoDriver.h"
#include "scene/CAnimatedMeshSceneNode.h"
#include "scene/CBillboardSceneNode.h"
#include "scene/CCameraSceneNode.h"
#include "scene/CLightSceneNode.h"
#include "scene/CSceneCollisionManager.h"
#include "scene/CSceneNodeAnimatorRotation.h"
#include "scene/CSkyBoxSceneNode.h"
#include "scene/CStaticMeshOBJ.h"
#include "scene/SViewFrustrum.h"

namespace daisy {
namespace scene {

ox::scene::ISceneManager* createSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
    ox::gui::ICursorControl* cursorControl)
{
    return new CSceneManager(driver, fs, cursorControl);
}

CSceneManager::DefaultNodeEntry::DefaultNodeEntry(ox::scene::ISceneNode* n)
    : node(n), textureValue(0)
{
    if (n->getMaterialCount())
        textureValue = n->getMaterial(0).Texture1;
}

//! The distance to the node's absolute translation, in double precision as Irrlicht computes it.
CSceneManager::TransparentNodeEntry::TransparentNodeEntry(ox::scene::ISceneNode* n, const vector3df& camera)
    : node(n), distance((float)getTranslation(n->getAbsoluteTransformation()).getDistanceFrom(camera))
{
}

//! The root node has no parent and no scene manager.
CSceneManager::CSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
    ox::gui::ICursorControl* cursorControl)
    : ox::scene::ISceneNode(0, 0), Driver(driver), FileSystem(fs), CursorControl(cursorControl), CollisionManager(0),
      ActiveCamera(0), ShadowColor(150, 0, 0, 0)
{
    if (Driver)
        Driver->grab();

    if (FileSystem)
        FileSystem->grab();

    if (CursorControl)
        CursorControl->grab();

    CollisionManager = new CSceneCollisionManager(this, Driver);
}

CSceneManager::~CSceneManager()
{
    clearDeletionList();

    if (Driver)
        Driver->drop();

    if (FileSystem)
        FileSystem->drop();

    if (CursorControl)
        CursorControl->drop();

    if (CollisionManager)
        CollisionManager->drop();

    removeAllMeshes();

    if (ActiveCamera)
        ActiveCamera->drop();
}

//! Looks the lowercased name up in the cache, else opens the file through the file system (so
//! aliases resolve) and loads it if its lowercased name contains ".obj".
ox::scene::IAnimatedMesh* CSceneManager::getMesh(const char* filename)
{
    ox::core::CString<char> name = filename;
    ox::core::CStringFunctions::ansiMakeLower(name);

    std::map<std::string, ox::scene::IAnimatedMesh*>::iterator cached = Meshes.find(name.c_str());
    if (cached != Meshes.end())
        return cached->second;

    ox::io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (!file)
    {
        os::Printer::log("Could not load mesh, because file could not be opened.", filename, ox::event::ELL_ERROR);
        return 0;
    }

    ox::scene::IAnimatedMesh* msh = 0;
    if (strstr(name.c_str(), ".obj") && strstr(file->getFileName(), ".obj"))
    {
        CStaticMeshOBJ* obj = new CStaticMeshOBJ();
        if (obj->loadFile(file))
        {
            Meshes[name.c_str()] = obj;
            msh = obj;
        }
        else
            obj->drop();
    }

    file->drop();

    if (!msh)
        os::Printer::log("Could not load mesh, file format seems to be unsupported", filename, ox::event::ELL_ERROR);
    else
        os::Printer::log("Loaded mesh", filename, ox::event::ELL_INFORMATION);

    return msh;
}

ox::video::IVideoDriver* CSceneManager::getVideoDriver()
{
    return Driver;
}

// The add functions parent new nodes to the root unless given a parent; the parent holds the
// only reference.

ox::scene::IAnimatedMeshSceneNode* CSceneManager::addAnimatedMeshSceneNode(ox::scene::IAnimatedMesh* mesh,
    ox::scene::ISceneNode* parent, int id, const vector3df& position, const vector3df& rotation,
    const vector3df& scale)
{
    if (!mesh)
        return 0;

    if (!parent)
        parent = this;

    ox::scene::IAnimatedMeshSceneNode* node =
        new CAnimatedMeshSceneNode(mesh, parent, this, id, position, rotation, scale);
    node->drop();
    return node;
}

//! Also makes the new camera the active one.
ox::scene::ICameraSceneNode* CSceneManager::addCameraSceneNode(ox::scene::ISceneNode* parent,
    const vector3df& position, const vector3df& lookat, int id)
{
    if (!parent)
        parent = this;

    ox::scene::ICameraSceneNode* node = new CCameraSceneNode(parent, this, id, position, lookat);
    node->drop();

    setActiveCamera(node);
    return node;
}

ox::scene::ILightSceneNode* CSceneManager::addLightSceneNode(ox::scene::ISceneNode* parent, const vector3df& position,
    ox::video::SColorf color, float radius, int id)
{
    if (!parent)
        parent = this;

    ox::scene::ILightSceneNode* node = new CLightSceneNode(parent, this, id, position, color, radius);
    node->drop();
    return node;
}

ox::scene::IBillboardSceneNode* CSceneManager::addBillboardSceneNode(ox::scene::ISceneNode* parent,
    const ox::core::CDimension2d<float>& size, const vector3df& position, int id)
{
    if (!parent)
        parent = this;

    ox::scene::IBillboardSceneNode* node = new CBillboardSceneNode(parent, this, id, position, size);
    node->drop();
    return node;
}

ox::scene::ISceneNode* CSceneManager::addSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom,
    ox::video::ITexture* left, ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back,
    ox::scene::ISceneNode* parent, int id)
{
    if (!parent)
        parent = this;

    ox::scene::ISceneNode* node = new CSkyBoxSceneNode(top, bottom, left, right, front, back, parent, this, id);
    node->drop();
    return node;
}

ox::scene::ISceneNode* CSceneManager::getRootSceneNode()
{
    return this;
}

//! Depth first, in child order.
ox::scene::ISceneNode* CSceneManager::getSceneNodeFromId(int id, ox::scene::ISceneNode* start)
{
    if (!start)
        start = getRootSceneNode();

    if (start->getID() == id)
        return start;

    const ox::TList<ox::scene::ISceneNode*>& list = start->getChildren();
    for (ox::TList<ox::scene::ISceneNode*>::const_iterator it = list.begin(); it != list.end(); ++it)
    {
        ox::scene::ISceneNode* node = getSceneNodeFromId(id, *it);
        if (node)
            return node;
    }

    return 0;
}

ox::scene::ICameraSceneNode* CSceneManager::getActiveCamera()
{
    return ActiveCamera;
}

void CSceneManager::setActiveCamera(ox::scene::ICameraSceneNode* camera)
{
    if (ActiveCamera)
        ActiveCamera->drop();

    ActiveCamera = camera;

    if (ActiveCamera)
        ActiveCamera->grab();
}

void CSceneManager::setShadowColor(ox::video::SColor color)
{
    ShadowColor = color;
}

ox::video::SColor CSceneManager::getShadowColor() const
{
    return ShadowColor;
}

bool CSceneManager::isCulled(ox::scene::ISceneNode* node)
{
    if (!node->getAutomaticCulling())
        return false;

    ox::scene::ICameraSceneNode* cam = getActiveCamera();
    if (!cam)
        return false;

    aabbox3df tbox = node->getBoundingBox();
    transformBox(node->getAbsoluteTransformation(), tbox);
    return !intersectsWithBox(tbox, cam->getViewFrustrum()->boundingBox);
}

//! A node of the default pass goes to the transparent list if any of its materials' renderers is
//! transparent, else to the opaque list; either way only if its box is in the view.
void CSceneManager::registerNodeForRendering(ox::scene::ISceneNode* node, ox::scene::E_SCENE_NODE_RENDER_TIME time)
{
    switch (time)
    {
    case ox::scene::SNRT_LIGHT_AND_CAMERA:
        LightAndCameraList.push_back(node);
        break;
    case ox::scene::SNRT_SKY_BOX:
        SkyBoxList.push_back(node);
        break;
    case ox::scene::SNRT_DEFAULT:
        {
            int count = node->getMaterialCount();
            for (int i = 0; i < count; ++i)
            {
                ox::video::IMaterialRenderer* rnd = Driver->getMaterialRenderer(node->getMaterial(i).MaterialType);
                if (rnd && rnd->isTransparent())
                {
                    if (!isCulled(node))
                        TransparentNodeList.push_back(TransparentNodeEntry(node, camTransPos));
                    return;
                }
            }

            if (!isCulled(node))
                DefaultNodeList.push_back(DefaultNodeEntry(node));
        }
        break;
    case ox::scene::SNRT_SHADOW:
        ShadowNodeList.push_back(node);
        break;
    }
}

//! Registration (OnPreRender), then cameras and lights, skyboxes, opaque nodes by texture,
//! shadows, transparent nodes nearest first, and finally the animators (OnPostRender).
void CSceneManager::drawAll()
{
    if (!Driver)
        return;

    camTransPos.set(0, 0, 0);
    if (ActiveCamera)
        camTransPos = ActiveCamera->getAbsolutePosition();

    OnPreRender();

    Driver->deleteAllDynamicLights();

    for (size_t i = 0; i < LightAndCameraList.size(); ++i)
        LightAndCameraList[i]->render();
    LightAndCameraList.clear();

    for (size_t i = 0; i < SkyBoxList.size(); ++i)
        SkyBoxList[i]->render();
    SkyBoxList.clear();

    std::sort(DefaultNodeList.begin(), DefaultNodeList.end());
    for (size_t i = 0; i < DefaultNodeList.size(); ++i)
        DefaultNodeList[i].node->render();
    DefaultNodeList.clear();

    for (size_t i = 0; i < ShadowNodeList.size(); ++i)
        ShadowNodeList[i]->render();
    if (!ShadowNodeList.empty())
        Driver->drawStencilShadow(true, ShadowColor, ShadowColor, ShadowColor, ShadowColor);
    ShadowNodeList.clear();

    std::sort(TransparentNodeList.begin(), TransparentNodeList.end());
    for (size_t i = 0; i < TransparentNodeList.size(); ++i)
        TransparentNodeList[i].node->render();
    TransparentNodeList.clear();

    OnPostRender(os::Timer::getTime());

    clearDeletionList();
}

ox::scene::ISceneNodeAnimator* CSceneManager::createRotationAnimator(const vector3df& rotationPerSecond)
{
    return new CSceneNodeAnimatorRotation(os::Timer::getTime(), rotationPerSecond);
}

ox::scene::ISceneCollisionManager* CSceneManager::getSceneCollisionManager()
{
    return CollisionManager;
}

void CSceneManager::addToDeletionQueue(ox::scene::ISceneNode* node)
{
    if (!node)
        return;

    node->grab();
    DeletionList.push_back(node);
}

void CSceneManager::clearDeletionList()
{
    for (size_t i = 0; i < DeletionList.size(); ++i)
    {
        DeletionList[i]->remove();
        DeletionList[i]->drop();
    }
    DeletionList.clear();
}

//! Forwards to the active camera, which absorbs nothing.
bool CSceneManager::postEventFromUser(ox::event::SEvent event)
{
    ox::scene::ICameraSceneNode* cam = getActiveCamera();
    if (cam)
        return cam->OnEvent(event);

    return false;
}

//! Removes every node and the active camera; the mesh cache stays.
void CSceneManager::clear()
{
    removeAll();
}

void CSceneManager::removeAll()
{
    ISceneNode::removeAll();
    setActiveCamera(0);
}

void CSceneManager::removeAllMeshes()
{
    for (std::map<std::string, ox::scene::IAnimatedMesh*>::iterator it = Meshes.begin(); it != Meshes.end(); ++it)
        it->second->drop();
    Meshes.clear();
}

void CSceneManager::render()
{
}

//! The root has no box; Irrlicht returns a null reference here.
const aabbox3df& CSceneManager::getBoundingBox() const
{
    static aabbox3df none;
    return none;
}

ox::scene::ISceneNode* CSceneManager::addTestSceneNode(float size, ox::scene::ISceneNode* parent, int id,
    const vector3df& position, const vector3df& rotation, const vector3df& scale)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addMeshSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent, int id,
    const vector3df& position, const vector3df& rotation, const vector3df& scale)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addWaterSurfaceSceneNode(ox::scene::IMesh* mesh, float waveHeight,
    float waveSpeed, float waveLength, ox::scene::ISceneNode* parent, int id, const vector3df& position,
    const vector3df& rotation, const vector3df& scale)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addBspTreeSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent, int id)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addOctTreeSceneNode(ox::scene::IAnimatedMesh* mesh,
    ox::scene::ISceneNode* parent, int id, int minimalPolysPerNode)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addOctTreeSceneNode(ox::scene::IMesh* mesh, ox::scene::ISceneNode* parent,
    int id, int minimalPolysPerNode)
{
    return 0;
}

ox::scene::ICameraSceneNode* CSceneManager::addCameraSceneNodeMaya(ox::scene::ISceneNode* parent, float rotateSpeed,
    float zoomSpeed, float translationSpeed, int id)
{
    return 0;
}

ox::scene::ICameraSceneNode* CSceneManager::addCameraSceneNodeFPS(ox::scene::ISceneNode* parent, float rotateSpeed,
    float moveSpeed, int id, ox::SKeyMap* keyMapArray, int keyMapSize)
{
    return 0;
}

ox::scene::IParticleSystemSceneNode* CSceneManager::addParticleSystemSceneNode(bool withDefaultEmitter,
    ox::scene::ISceneNode* parent, int id, const vector3df& position, const vector3df& rotation,
    const vector3df& scale)
{
    return 0;
}

ox::scene::ITerrainSceneNode* CSceneManager::addTerrainSceneNode(ox::scene::ISceneNode* parent, int id,
    ox::video::IImage* texture, ox::video::IImage* heightmap, ox::video::ITexture* detailmap,
    const ox::core::CDimension2d<float>& stretchSize, float maxHeight,
    const ox::core::CDimension2d<int>& defaultVertexBlockSize)
{
    return 0;
}

ox::scene::ISceneNode* CSceneManager::addEmptySceneNode(ox::scene::ISceneNode* parent, int id)
{
    return 0;
}

ox::scene::IDummyTransformationSceneNode* CSceneManager::addDummyTransformationSceneNode(
    ox::scene::ISceneNode* parent, int id)
{
    return 0;
}

ox::scene::ITextSceneNode* CSceneManager::addTextSceneNode(ox::gui::IGUIFont* font, const wchar_t* text,
    ox::video::SColor color, ox::scene::ISceneNode* parent, const vector3df& position, int id)
{
    return 0;
}

ox::scene::IAnimatedMesh* CSceneManager::addHillPlaneMesh(const char* name, const ox::core::CDimension2d<float>& tileSize,
    const ox::core::CDimension2d<int>& tileCount, ox::video::SMaterial* material, float hillHeight,
    const ox::core::CDimension2d<float>& countHills, const ox::core::CDimension2d<float>& textureRepeatCount)
{
    return 0;
}

ox::scene::IAnimatedMesh* CSceneManager::addTerrainMesh(const char* meshname, ox::video::IImage* texture,
    ox::video::IImage* heightmap, const ox::core::CDimension2d<float>& stretchSize, float maxHeight,
    const ox::core::CDimension2d<int>& defaultVertexBlockSize)
{
    return 0;
}

ox::scene::ISceneNodeAnimator* CSceneManager::createFlyCircleAnimator(const vector3df& center, float radius,
    float speed)
{
    return 0;
}

ox::scene::ISceneNodeAnimator* CSceneManager::createFlyStraightAnimator(const vector3df& startPoint,
    const vector3df& endPoint, unsigned int timeForWay, bool loop)
{
    return 0;
}

ox::scene::ISceneNodeAnimator* CSceneManager::createTextureAnimator(const ox::TArray<ox::video::ITexture*>& textures,
    int timePerFrame, bool loop)
{
    return 0;
}

ox::scene::ISceneNodeAnimator* CSceneManager::createDeleteAnimator(unsigned int timeMs)
{
    return 0;
}

ox::scene::ISceneNodeAnimatorCollisionResponse* CSceneManager::createCollisionResponseAnimator(
    ox::scene::ITriangleSelector* world, ox::scene::ISceneNode* sceneNode, const vector3df& ellipsoidRadius,
    const vector3df& gravityPerSecond, float accelerationPerSecond, const vector3df& ellipsoidTranslation,
    float slidingValue)
{
    return 0;
}

ox::scene::ISceneNodeAnimator* CSceneManager::createFollowSplineAnimator(int startTime,
    const ox::TArray<vector3df>& points, float speed, float tightness)
{
    return 0;
}

ox::scene::ITriangleSelector* CSceneManager::createTriangleSelector(ox::scene::IMesh* mesh,
    ox::scene::ISceneNode* node)
{
    return 0;
}

ox::scene::ITriangleSelector* CSceneManager::createTriangleSelectorFromBoundingBox(ox::scene::ISceneNode* node)
{
    return 0;
}

ox::scene::ITriangleSelector* CSceneManager::createOctTreeTriangleSelector(ox::scene::IMesh* mesh,
    ox::scene::ISceneNode* node, int minimalPolysPerNode)
{
    return 0;
}

ox::scene::IMetaTriangleSelector* CSceneManager::createMetaTriangleSelector()
{
    return 0;
}

void CSceneManager::addExternalMeshLoader(ox::scene::IMeshLoader* externalLoader)
{
}

ox::scene::IMeshManipulator* CSceneManager::getMeshManipulator()
{
    return 0;
}

} // end namespace scene
} // end namespace daisy
