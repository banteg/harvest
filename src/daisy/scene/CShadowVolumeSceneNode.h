// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CShadowVolumeSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__
#define __C_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IShadowVolumeSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"
#include "ox/video/SLight.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	//! Scene node for rendering a shadow volume into a stencil buffer.
	class CShadowVolumeSceneNode : public IShadowVolumeSceneNode
	{
	public:

		//! constructor
		CShadowVolumeSceneNode(ISceneNode* parent, ISceneManager* mgr, 
			int id,  bool zfailmethod=true, float infinity=10000.0f);

		//! destructor
		virtual ~CShadowVolumeSceneNode();

		//! sets the mesh from which the shadow volume should be generated.
		virtual void setMeshToRenderFrom(IMesh* mesh);

		//! pre render method
		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! returns the material based on the zero based index i.
		virtual ox::video::SMaterial& getMaterial(int i);
		
		//! returns amount of materials used by this scene node.
		virtual int getMaterialCount();

	private:

		struct SShadowVolume
		{
			ox::core::CVector3d<float>* vertices;
			int count;
			int size;
		};

		void createShadowVolume(const ox::core::CVector3d<float>& pos);
		void createZPassVolume(int faceCount, int& numEdges, const ox::core::CVector3d<float> light, SShadowVolume* svp, bool caps);
		void createZFailVolume(int faceCount, int& numEdges, const ox::core::CVector3d<float> light, SShadowVolume* svp);
		void addEdge(int& numEdges, unsigned short v0, unsigned short v1);

		//! Generates adjacency information based on mesh indices.
		void calculateAdjacency(float epsilon=0.0001f);

		bool UseZFailMethod;
		ox::core::CAabbox3d<float> Box;

		unsigned short* Indices;
		ox::core::CVector3d<float>* Vertices;
		unsigned short* Adjacency;
		bool* FaceData; // used for zfail method, if face is front facing

		int IndexCountAllocated;
		int VertexCountAllocated;
		int IndexCount;
		int VertexCount;	

		ox::TArray<SShadowVolume> ShadowVolumes; // a shadow volume for every light
		int ShadowVolumesUsed;

		unsigned short* Edges;
		int EdgeCount;

		float Infinity;
	};

} // end namespace scene
} // end namespace daisy

#endif

