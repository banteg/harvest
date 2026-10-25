// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_MANAGER_H_INCLUDED__
#define __C_SCENE_MANAGER_H_INCLUDED__

#include "ox/scene/ISceneManager.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/SColor.h"
#include "ox/video/SLight.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/IBillboardSceneNode.h"
#include "ox/scene/IBspTreeSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/IDummyTransformationSceneNode.h"
#include "ox/scene/ILightSceneNode.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshManipulator.h"
#include "ox/scene/IMetaTriangleSelector.h"
#include "ox/scene/IParticleSystemSceneNode.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/scene/ISceneNodeAnimatorCollisionResponse.h"
#include "ox/scene/ITerrainSceneNode.h"
#include "ox/scene/ITextSceneNode.h"
#include "ox/scene/ITriangleSelector.h"
#include "ox/io/IFileSystem.h"
#include "ox/gui/IGUIFont.h"
#include "ox/event/IEventReceiver.h"
#include "daisy/scene/SKeyMap.h"
#include "ox/scene/ISceneNode.h"
#include "ox/gui/ICursorControl.h"
#include "ox/core/CString.h"
#include "ox/TArray.h"
#include "ox/scene/IMeshLoader.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	/*!
		The Scene Manager manages scene nodes, mesh recources, cameras and all the other stuff.
	*/
	class CSceneManager : public ISceneManager, ISceneNode
	{
	public:

		//! constructor
		CSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs, ox::gui::ICursorControl* cursorControl);

		//! destructor
		virtual ~CSceneManager();

		//! gets an animateable mesh. loads it if needed. returned pointer must not be dropped.
		virtual IAnimatedMesh* getMesh(const char* filename);

		//! returns the video driver
		virtual ox::video::IVideoDriver* getVideoDriver();

		//! adds a test scene node for test purposes to the scene. It is a simple cube of (1,1,1) size.
		//! the returned pointer must not be dropped.
		virtual ISceneNode* addTestSceneNode(float size=10.0f, ISceneNode* parent=0, int id=-1,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! adds a scene node for rendering an animated mesh model
		virtual IAnimatedMeshSceneNode* addAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent=0, int id=-1,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! adds a scene node for rendering a static mesh
		//! the returned pointer must not be dropped.
		virtual ISceneNode* addMeshSceneNode(IMesh* mesh, ISceneNode* parent=0, int id=-1,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),	const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! Adds a scene node for rendering a animated water surface mesh.
		virtual ISceneNode* addWaterSurfaceSceneNode(IMesh* mesh, float waveHeight, float waveSpeed, float wlenght, ISceneNode* parent=0, int id=-1,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! registers a node for rendering it at a specific time.
		virtual void registerNodeForRendering(ISceneNode* node, E_SCENE_NODE_RENDER_TIME = SNRT_DEFAULT);

		//! draws all scene nodes
		virtual void drawAll();

		//! Adds a scene node for rendering using a binary space partition tree.
		virtual IBspTreeSceneNode* addBspTreeSceneNode(IMesh* mesh, ISceneNode* parent=0, int id=-1);

		//! Adds a scene node for rendering using a octtree to the scene graph. This a good method for rendering
		//! scenes with lots of geometry. The Octree is built on the fly from the mesh, much
		//! faster then a bsp tree.
		virtual ISceneNode* addOctTreeSceneNode(IAnimatedMesh* mesh, ISceneNode* parent=0,
			int id=-1, int minimalPolysPerNode=128);

		//! Adss a scene node for rendering using a octtree. This a good method for rendering
		//! scenes with lots of geometry. The Octree is built on the fly from the mesh, much
		//! faster then a bsp tree.
		virtual ISceneNode* addOctTreeSceneNode(IMesh* mesh, ISceneNode* parent=0,
			int id=-1, int minimalPolysPerNode=128);

		//! Adds a camera scene node to the tree and sets it as active camera.
		//! \param position: Position of the space relative to its parent where the camera will be placed.
		//! \param lookat: Position where the camera will look at. Also known as target.
		//! \param parent: Parent scene node of the camera. Can be null. If the parent moves,
		//! the camera will move too.
		//! \return Returns pointer to interface to camera
		virtual ICameraSceneNode* addCameraSceneNode(ISceneNode* parent = 0,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& lookat = ox::core::CVector3d<float>(0,0,0), int id=-1);

		//! Adds a camera scene node which is able to be controlle with the mouse similar
		//! like in the 3D Software Maya by Alias Wavefront.
		//! The returned pointer must not be dropped.
		virtual ICameraSceneNode* addCameraSceneNodeMaya(ISceneNode* parent = 0,
			float rotateSpeed = -1500.0f, float zoomSpeed = 200.0f, float translationSpeed = 100.0f, int id=-1);

		//! Adds a camera scene node which is able to be controled with the mouse and keys
		//! like in most first person shooters (FPS):
		virtual ICameraSceneNode* addCameraSceneNodeFPS(ISceneNode* parent = 0,
			float rotateSpeed = 1500.0f, float moveSpeed = 200.0f, int id=-1,
			SKeyMap* keyMapArray=0, int keyMapSize=0);

		//! Adds a dynamic light scene node. The light will cast dynamic light on all
		//! other scene nodes in the scene, which have the material flag video::MTF_LIGHTING
		//! turned on. (This is the default setting in most scene nodes).
		virtual ILightSceneNode* addLightSceneNode(ISceneNode* parent = 0,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			ox::video::SColorf color = ox::video::SColorf(1.0f, 1.0f, 1.0f),	float range=100.0f, int id=-1);

		//! Adds a billboard scene node to the scene. ox::core::A billboard is like a 3d sprite: ox::core::A 2d element,
		//! which always looks to the camera. It is usually used for things like explosions, fire,
		//! lensflares and things like that.
		virtual IBillboardSceneNode* addBillboardSceneNode(ISceneNode* parent = 0,
			const ox::core::CDimension2d<float>& size = ox::core::CDimension2d<float>(10.0f, 10.0f),
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0), int id=-1);

		//! Adds a skybox scene node. ox::core::A skybox is a big cube with 6 textures on it and
		//! is drawed around the camera position.
		virtual ISceneNode* addSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom,
			ox::video::ITexture* left, ox::video::ITexture* right, ox::video::ITexture* front,
			ox::video::ITexture* back, ISceneNode* parent = 0, int id=-1);

		//! Adds a text scene node, which is able to display
		//! 2d text at a position in three dimensional space
		virtual ITextSceneNode* addTextSceneNode(ox::gui::IGUIFont* font, const wchar_t* text,
			ox::video::SColor color=ox::video::SColor(100,255,255,255),
			ISceneNode* parent = 0,	const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			int id=-1);

		//! Adds a Hill Plane mesh to the mesh pool. The mesh is generated on the fly
		//! and looks like a plane with some hills on it. It is uses mostly for quick
		//! tests of the engine only. You can specify how many hills there should be
		//! on the plane and how high they should be. Also you must specify a name for
		//! the mesh, because the mesh is added to the mesh pool, and can be retieved
		//! again using ISceneManager::getMesh with the name as parameter.
		virtual IAnimatedMesh* addHillPlaneMesh(const char* name,
			const ox::core::CDimension2d<float>& tileSize, const ox::core::CDimension2d<int>& tileCount,
			ox::video::SMaterial* material = 0,	float hillHeight = 0.0f,
			const ox::core::CDimension2d<float>& countHills = ox::core::CDimension2d<float>(0.0f, 0.0f),
			const ox::core::CDimension2d<float>& textureRepeatCount = ox::core::CDimension2d<float>(1.0f, 1.0f));

		//! Adds a terrain mesh to the mesh pool.
		virtual IAnimatedMesh* addTerrainMesh(const char* meshname,	ox::video::IImage* texture, ox::video::IImage* heightmap,
			const ox::core::CDimension2d<float>& stretchSize,
			float maxHeight, const ox::core::CDimension2d<int>& defaultVertexBlockSize);

		//! Adds a particle system scene node.
		virtual IParticleSystemSceneNode* addParticleSystemSceneNode(
			bool withDefaultEmitter=true, ISceneNode* parent=0, int id=-1,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! Adds a terrain scene node to the scene graph.
		virtual ITerrainSceneNode* addTerrainSceneNode(ISceneNode* parent, int id,
			ox::video::IImage* texture,	ox::video::IImage* heightmap, ox::video::ITexture* detailmap,
			const ox::core::CDimension2d<float>& stretchSize = ox::core::CDimension2d<float>(10.0f,10.0f),
			float maxHeight=200.0f,
			const ox::core::CDimension2d<int>& defaultVertexBlockSize = ox::core::CDimension2d<int>(64,64));

		//! Adds a dummy transformation scene node to the scene graph.
		virtual IDummyTransformationSceneNode* addDummyTransformationSceneNode(
			ISceneNode* parent=0, int id=-1);

		//! Adds an empty scene node.
		virtual ISceneNode* addEmptySceneNode(ISceneNode* parent, int id=-1);

		//! Returns the root scene node. This is the scene node wich is parent
		//! of all scene nodes. The root scene node is a special scene node which
		//! only exists to manage all scene nodes. It is not rendered and cannot
		//! be removed from the scene.
		//! \return Returns a pointer to the root scene node.
		virtual ISceneNode* getRootSceneNode();

		//! Returns the current active camera.
		//! \return The active camera is returned. Note that this can be NULL, if there
		//! was no camera created yet.
		virtual ICameraSceneNode* getActiveCamera();

		//! Sets the active camera. The previous active camera will be deactivated.
		//! \param camera: The new camera which should be active.
		virtual void setActiveCamera(ICameraSceneNode* camera);

		//! creates a rotation animator, which rotates the attached scene node around itself.
		//! \param rotationPerSecond: Specifies the speed of the animation
		//! \return Returns the animator. Attach it to a scene node with ISceneNode::addAnimator()
		//! and the animator will animate it.
		virtual ISceneNodeAnimator* createRotationAnimator(const ox::core::CVector3d<float>& rotationPerSecond);

		//! creates a fly circle animator, which lets the attached scene node fly
		//! around a center. The center is the position of the scene node.
		//! \param rotationSpeed:
		//! \return Returns the animator. Attach it to a scene node with ISceneNode::addAnimator()
		//! and the animator will animate it.
		virtual ISceneNodeAnimator* createFlyCircleAnimator(const ox::core::CVector3d<float>& normal, float radius, float speed);

		//! Creates a fly straight animator, which lets the attached scene node
		//! fly or move along a line between two points.
		virtual ISceneNodeAnimator* createFlyStraightAnimator(const ox::core::CVector3d<float>& startPoint,
			const ox::core::CVector3d<float>& endPoint, unsigned int timeForWay, bool loop=false);

		//! Creates a texture animator, which switches the textures of the target scene
		//! node based on a list of textures.
		virtual ISceneNodeAnimator* createTextureAnimator(const ox::TArray<ox::video::ITexture*>& textures,
			int timePerFrame, bool loop);

		//! Creates a scene node animator, which deletes the scene node after
		//! some time automaticly.
		virtual ISceneNodeAnimator* createDeleteAnimator(unsigned int timeMS);


		//! Creates a special scene node animator for doing automatic collision detection
		//! and response.
		virtual ISceneNodeAnimatorCollisionResponse* createCollisionResponseAnimator(
			ITriangleSelector* world, ISceneNode* sceneNode,
			const ox::core::CVector3d<float>& ellipsoidRadius = ox::core::CVector3d<float>(30,60,30),
			const ox::core::CVector3d<float>& gravityPerSecond = ox::core::CVector3d<float>(0,-100.0f,0),
			float accelerationPerSecond=100.0f,
			const ox::core::CVector3d<float>& ellipsoidTranslation = ox::core::CVector3d<float>(0,0,0),
			float slidingValue = 0.0005f);

		//! Creates a follow spline animator.
        virtual ISceneNodeAnimator* createFollowSplineAnimator(int startTime,
			const ox::TArray< ox::core::CVector3d<float> >& points,
			float speed = 1.0f, float tightness = 0.5f);

		//! Creates a simple ITriangleSelector, based on a mesh.
		virtual ITriangleSelector* createTriangleSelector(IMesh* mesh, ISceneNode* node);

		//! Creates a simple ITriangleSelector, based on a mesh.
		virtual ITriangleSelector* createOctTreeTriangleSelector(IMesh* mesh,
			ISceneNode* node, int minimalPolysPerNode);

		//! Creates a simple dynamic ITriangleSelector, based on a axis aligned bounding box.
		virtual ITriangleSelector* createTriangleSelectorFromBoundingBox(
			ISceneNode* node);

		//! Creates a meta triangle selector.
		virtual IMetaTriangleSelector* createMetaTriangleSelector();

		//! Adds an external mesh loader.
		virtual void addExternalMeshLoader(IMeshLoader* externalLoader);

		//! Returns a pointer to the scene collision manager.
		virtual ISceneCollisionManager* getSceneCollisionManager();

		//! Returns a pointer to the mesh manipulator.
		virtual IMeshManipulator* getMeshManipulator();

		//! Sets the color of stencil buffers shadows drawed by the scene manager.
		virtual void setShadowColor(ox::video::SColor color);

		//! Returns the current color of shadows.
		virtual ox::video::SColor getShadowColor() const;

		//! Adds a scene node to the deletion queue.
		virtual void addToDeletionQueue(ISceneNode* node);

		//! Returns the first scene node with the specified id.
		virtual ISceneNode* getSceneNodeFromId(int id, ISceneNode* start=0);

		//! Posts an input event to the environment. Usually you do not have to
		//! use this method, it is used by the internal engine.
		virtual bool postEventFromUser(ox::event::SEvent event);

		//! Clears the whole scene. All scene nodes are removed.
		virtual void clear();

		//! Removes all children of this scene node
		virtual void removeAll();

		virtual void removeAllMeshes();

	private:

		//! adds a mesh to the list
		void addMesh(const char* filename, IAnimatedMesh* mesh);

		//! returns if a mesh already was loaded
		bool wasMeshLoaded(const char* filename);

		//! returns an already loaded mesh
		IAnimatedMesh* findMesh(const char* lowerMadeFilename);

		//! returns if node is culled
		bool isCulled(ISceneNode* node);

		//! clears the deletion list
		void clearDeletionList();


		struct MeshEntry
		{
			ox::core::CString<char> Name;
			IAnimatedMesh* Mesh;

			bool operator < (const MeshEntry& other) const
			{
				return (Name < other.Name);
			}
		};

		struct DefaultNodeEntry
		{
			DefaultNodeEntry() {};

			DefaultNodeEntry(ISceneNode* n)
			{
				textureValue = 0;

				if (n->getMaterialCount())
					textureValue = (int)(long)(n->getMaterial(0).Texture1);

				node = n;
			}

			ISceneNode* node;
			int textureValue;

			bool operator < (const DefaultNodeEntry& other) const
			{
				return (textureValue < other.textureValue);
			}
		};


		struct TransparentNodeEntry
		{
			TransparentNodeEntry() {};

			TransparentNodeEntry(ISceneNode* n, ox::core::CVector3d<float> camera)
			{
				node = n;

				// TODO: this could be optimized, by not using sqrt
				distance = (float)(node->getAbsoluteTransformation().getTranslation().getDistanceFrom(camera));
			}

			ISceneNode* node;
			float distance;

			bool operator < (const TransparentNodeEntry& other) const
			{
				return (distance < other.distance);
			}
		};

		//! loaded meshes
		ox::TArray<MeshEntry> Meshes;

		//! video driver
		ox::video::IVideoDriver* Driver;

		//! file system
		ox::io::IFileSystem* FileSystem;

		//! cursor control
		ox::gui::ICursorControl* CursorControl;

		//! collision manager
		ISceneCollisionManager* CollisionManager;

		//! mesh manipulator
		IMeshManipulator* MeshManipulator;

		//! render pass lists
		ox::TArray<ISceneNode*> LightAndCameraList;
		ox::TArray<ISceneNode*> ShadowNodeList;
		ox::TArray<ISceneNode*> SkyBoxList;
		ox::TArray<DefaultNodeEntry> DefaultNodeList;
		ox::TArray<TransparentNodeEntry> TransparentNodeList;

		ox::TArray<IMeshLoader*> MeshLoaderList;
		ox::TArray<ISceneNode*> DeletionList;

		//! current active camera
		ICameraSceneNode* ActiveCamera;
		ox::core::CVector3d<float> camTransPos; // Position of camera for transparent nodes.

		ox::video::SColor ShadowColor;

	};

} // end namespace video
} // end namespace scene

#endif
