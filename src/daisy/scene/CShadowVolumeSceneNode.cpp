// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CShadowVolumeSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SLight.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/IShadowVolumeSceneNode.h"
#include "CShadowVolumeSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CShadowVolumeSceneNode::CShadowVolumeSceneNode(ISceneNode* parent,
											   ISceneManager* mgr,
											   int id,
											    bool zfailmethod,
												float infinity)
: IShadowVolumeSceneNode(parent, mgr, id), Edges(0), EdgeCount(0),
	ShadowVolumesUsed(0), Indices(0), Vertices(0), IndexCount(0),
	VertexCount(0),	IndexCountAllocated(0),
	VertexCountAllocated(0), UseZFailMethod(zfailmethod),
	Adjacency(0), FaceData(0), Infinity(infinity)
{
	AutomaticCullingEnabled = false;
}



//! destructor
CShadowVolumeSceneNode::~CShadowVolumeSceneNode()
{
	if (Edges)
		delete [] Edges;

	for (unsigned int i=0; i<ShadowVolumes.size(); ++i)
		delete [] ShadowVolumes[i].vertices;

	delete [] Vertices;
	delete [] Indices;
	delete [] Adjacency;
	delete [] FaceData;
}


void CShadowVolumeSceneNode::createShadowVolume(const ox::core::CVector3d<float>& light)
{
	SShadowVolume* svp = 0;

	// builds the shadow volume and adds it to the shadow volume list.

	if (ShadowVolumes.size() > (unsigned int)ShadowVolumesUsed)
	{
		// get the next unused buffer
		svp = &ShadowVolumes[ShadowVolumesUsed];
		if (svp->size >= IndexCount*5)
			svp->count = 0;
		else
		{
			svp->size = IndexCount*5;
			svp->count = 0;
			delete [] svp->vertices;
			svp->vertices = new ox::core::CVector3d<float>[svp->size];
		}

		++ShadowVolumesUsed;
	}
	else
	{
		// add a buffer
		SShadowVolume tmp;
		ShadowVolumes.push_back(tmp);
		svp = &ShadowVolumes[ShadowVolumes.size()-1];
		++ShadowVolumesUsed;

		// wir machen mal einen ziemlich großen shadowbuffer
		svp->size = IndexCount*5;
		svp->count = 0;
		svp->vertices = new ox::core::CVector3d<float>[svp->size];
	}

	int faceCount = (int)(IndexCount / 3);

	if (faceCount * 6 > EdgeCount || !Edges)
	{
		delete [] Edges;
		EdgeCount = faceCount * 6;
		Edges = new unsigned short[EdgeCount];
	}



	int numEdges = 0;
	ox::core::CVector3d<float> ls = light * Infinity; // light scaled

	//if (!UseZFailMethod)
	//	createZPassVolume(faceCount, numEdges, light, svp);
	//else
	//	createZFailVolume(faceCount, numEdges, light, svp);

	// the createZFailVolume does currently not work 100% correctly,
	// so we create createZPassVolume with caps if the zfail method
	// is used
	createZPassVolume(faceCount, numEdges, light, svp, UseZFailMethod);


	for (int i=0; i<numEdges; ++i)
    {
		ox::core::CVector3d<float> &v1 = Vertices[Edges[2*i+0]];
		ox::core::CVector3d<float> &v2 = Vertices[Edges[2*i+1]];
		ox::core::CVector3d<float> v3(v1 - ls);
		ox::core::CVector3d<float> v4(v2 - ls);

		// Add a quad (two triangles) to the vertex list
		svp->vertices[svp->count++] = v1;
		svp->vertices[svp->count++] = v2;
		svp->vertices[svp->count++] = v3;

		svp->vertices[svp->count++] = v2;
		svp->vertices[svp->count++] = v4;
		svp->vertices[svp->count++] = v3;
	}
}

void CShadowVolumeSceneNode::createZFailVolume(int faceCount, int& numEdges,
											   const ox::core::CVector3d<float> light,
											   SShadowVolume* svp)
{
	unsigned short wFace0, wFace1, wFace2;
	ox::core::CVector3d<float> ls = light * Infinity; // light scaled
	int i;

	// Check every face if it is front or back facing the light.
    for (i=0; i<faceCount; ++i)
    {
        wFace0 = Indices[3*i+0];
        wFace1 = Indices[3*i+1];
        wFace2 = Indices[3*i+2];

		ox::core::CVector3d<float> v0 = Vertices[wFace0];
        ox::core::CVector3d<float> v1 = Vertices[wFace1];
        ox::core::CVector3d<float> v2 = Vertices[wFace2];

		ox::core::CVector3d<float> normal = (v2-v1).crossProduct(v1-v0);

		if(normal.dotProduct(light) >= 0.0f )
		{
			FaceData[i] = false; // it's a back facing face

			// add front cap
			svp->vertices[svp->count++] = v0;
			svp->vertices[svp->count++] = v2;
			svp->vertices[svp->count++] = v1;

			// add back cap
			svp->vertices[svp->count++] = v0 - ls;
			svp->vertices[svp->count++] = v1 - ls;
			svp->vertices[svp->count++] = v2 - ls;
		}
		else
			FaceData[i] = true; // it's a front facing face
	}


    for(i=0; i<faceCount; ++i)
    {
		if (FaceData[i] == true)
		{
			wFace0 = Indices[3*i+0];
			wFace1 = Indices[3*i+1];
			wFace2 = Indices[3*i+2];

			unsigned short adj0 = Adjacency[3*i+0];
			unsigned short adj1 = Adjacency[3*i+1];
			unsigned short adj2 = Adjacency[3*i+2];

			if (adj0 != (unsigned short)-1 && FaceData[adj0] == false)
			{
				//	add edge v0-v1
			    Edges[2*numEdges+0] = wFace0;
				Edges[2*numEdges+1] = wFace1;
				++numEdges;
			}

			if (adj1 != (unsigned short)-1 && FaceData[adj1] == false)
			{
				//	add edge v1-v2
			    Edges[2*numEdges+0] = wFace1;
				Edges[2*numEdges+1] = wFace2;
				++numEdges;
			}

			if (adj2 != (unsigned short)-1 && FaceData[adj2] == false)
			{
				//	add edge v2-v0
			    Edges[2*numEdges+0] = wFace2;
				Edges[2*numEdges+1] = wFace0;
				++numEdges;
			}
		}
	}

}

void CShadowVolumeSceneNode::createZPassVolume(int faceCount,
											   int& numEdges,
											   const ox::core::CVector3d<float> lightsource,
											   SShadowVolume* svp, bool caps)
{
	ox::core::CVector3d<float> light = lightsource;
    if (light == ox::core::CVector3d<float>(0,0,0))
		light += ox::core::CVector3d<float>(0.0001f, 0.0001f, 0.0001f);

	ox::core::CVector3d<float> normal;
	unsigned short wFace0, wFace1, wFace2;
	ox::core::CVector3d<float> ls = light * Infinity; // light scaled

	for (int i=0; i<faceCount; ++i)
	{
		wFace0 = Indices[3*i+0];
        wFace1 = Indices[3*i+1];
		wFace2 = Indices[3*i+2];

		ox::core::CVector3d<float> v0(Vertices[wFace2] - Vertices[wFace1]);
		ox::core::CVector3d<float> v1(Vertices[wFace1] - Vertices[wFace0]);

		normal = v0.crossProduct(v1);

		if (normal.dotProduct(light) >= 0.0f)
		{
			Edges[2*numEdges+0] = wFace0;
			Edges[2*numEdges+1] = wFace1;
			++numEdges;

			Edges[2*numEdges+0] = wFace1;
			Edges[2*numEdges+1] = wFace2;
			++numEdges;

			Edges[2*numEdges+0] = wFace2;
			Edges[2*numEdges+1] = wFace0;
			++numEdges;

			if (caps)
			{
				svp->vertices[svp->count++] = Vertices[wFace0];
				svp->vertices[svp->count++] = Vertices[wFace2];
				svp->vertices[svp->count++] = Vertices[wFace1];

				svp->vertices[svp->count++] = Vertices[wFace0] - ls;
				svp->vertices[svp->count++] = Vertices[wFace1] - ls;
				svp->vertices[svp->count++] = Vertices[wFace2] - ls;
			}
		}
	}
}

//! sets the mesh from which the shadow volume should be generated.
void CShadowVolumeSceneNode::setMeshToRenderFrom(IMesh* mesh)
{
	ShadowVolumesUsed = 0;

	int oldIndexCount = IndexCount;
	int oldVertexCount = VertexCount;

	VertexCount = 0;
	IndexCount = 0;

	if (!mesh)
		return;

	// calculate total amount of vertices and indices

	int i;
	int totalVertices = 0;
	int totalIndices = 0;
	int bufcnt = mesh->getMeshBufferCount();
	IMeshBuffer* b;

	for (i=0; i<bufcnt; ++i)
	{
		b = mesh->getMeshBuffer(i);
		totalIndices += b->getIndexCount();
		totalVertices += b->getVertexCount();
	}

	// allocate memory if nececcary

	if (totalVertices > VertexCountAllocated)
	{
		delete [] Vertices;
		Vertices = new ox::core::CVector3d<float>[totalVertices];
		VertexCountAllocated = totalVertices;
	}

	if (totalIndices > IndexCountAllocated)
	{
		delete [] Indices;
		Indices = new unsigned short[totalIndices];
		IndexCountAllocated = totalIndices;

		if (UseZFailMethod)
		{
			delete [] FaceData;
			FaceData = new bool[totalIndices / 3];
		}
	}

	// copy mesh

	for (i=0; i<bufcnt; ++i)
	{
		b = mesh->getMeshBuffer(i);

		int idxcnt = b->getIndexCount();
		int vtxnow = VertexCount;

		const unsigned short* idxp = b->getIndices();
		const unsigned short* idxpend = idxp + idxcnt;

		for (; idxp!=idxpend; ++idxp)
			Indices[IndexCount++] = *idxp + vtxnow;

		int vtxcnt = b->getVertexCount();

		switch((int)b->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				const ox::video::S3DVertex* vp = (ox::video::S3DVertex*)b->getVertices();
				const ox::video::S3DVertex* vpend = vp + vtxcnt;

				for (; vp!=vpend; ++vp)
					Vertices[VertexCount++] = (*vp).Pos;
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				const ox::video::S3DVertex2TCoords* vp = (ox::video::S3DVertex2TCoords*)b->getVertices();
				const ox::video::S3DVertex2TCoords* vpend = vp + vtxcnt;

				for (; vp!=vpend; ++vp)
					Vertices[VertexCount++] = (*vp).Pos;
			}
			break;
		}
	}

	// recalculate adjacency if neccessarry
	if (oldVertexCount != VertexCount &&
		oldIndexCount != IndexCount && UseZFailMethod)
		calculateAdjacency();

	// create as much shadow volumes as there are lights but
	// do not ignore the max light settings.

	int lights = SceneManager->getVideoDriver()->getDynamicLightCount();
	ox::core::CMatrix4 mat = Parent->getAbsoluteTransformation();
	ox::core::CVector3d<float> parentpos = Parent->getAbsolutePosition();
	ox::core::CVector3d<float> lpos;
	mat.makeInverse();

	for (i=0; i<lights; ++i)
	{
		const ox::video::SLight& dl = SceneManager->getVideoDriver()->getDynamicLight(i);
		lpos = dl.Position;
		if (dl.CastShadows &&
			fabs((lpos - parentpos).getLengthSQ()) <= (dl.Radius*dl.Radius*4.0f))
		{
			mat.transformVect(lpos);
			createShadowVolume(lpos);
		}
	}
}



//! pre render method
void CShadowVolumeSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, ox::scene::SNRT_SHADOW);
}



//! renders the node.
void CShadowVolumeSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (!ShadowVolumesUsed || !driver)
		return;

	driver->setTransform(ox::video::ETS_WORLD, Parent->getAbsoluteTransformation());

	for (int i=0; i<ShadowVolumesUsed; ++i)
		driver->drawStencilShadowVolume(ShadowVolumes[i].vertices,
			ShadowVolumes[i].count, UseZFailMethod);
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CShadowVolumeSceneNode::getBoundingBox() const
{
	return Box;
}



//! returns the material based on the zero based index i.
ox::video::SMaterial& CShadowVolumeSceneNode::getMaterial(int i)
{
#ifdef HARVEST_PORT
	// A material-less node still needs a valid reference for callers of the node interface.
	return ISceneNode::getMaterial(i);
#else
	// this should never be called, because a shadow volume has got no
	// material
	return *((ox::video::SMaterial*)(0));
#endif

}



//! returns amount of materials used by this scene node.
int CShadowVolumeSceneNode::getMaterialCount()
{
	return 0;
}


//! Generates adjacency information based on mesh indices.
void CShadowVolumeSceneNode::calculateAdjacency(float epsilon)
{
	delete [] Adjacency;
	Adjacency = new unsigned short[IndexCount];

	epsilon *= epsilon;

	float t = 0;

	// go through all faces and fetch their three neighbours
	for (int f=0; f<IndexCount; f+=3)
	{
		for (int edge = 0; edge<3; ++edge)
		{
			ox::core::CVector3d<float> v1 = Vertices[Indices[f+edge]];
			ox::core::CVector3d<float> v2 = Vertices[Indices[f+((edge+1)%3)]];

			// now we search an_O_ther _F_ace with these two
			// vertices, which is not the current face.

			int of;

			for (of=0; of<IndexCount; of+=3)
				if (of != f)
				{
					int cnt1 = 0;
					int cnt2 = 0;

					for (int e=0; e<3; ++e)
					{
						t = v1.getDistanceFromSQ(Vertices[Indices[of+e]]);
						if (t <= epsilon && t >= -epsilon)
							++cnt1;

						t = v2.getDistanceFromSQ(Vertices[Indices[of+e]]);
						if (t <= epsilon && t >= -epsilon)
							++cnt2;
					}

					if (cnt1 == 1 && cnt2 == 1)
						break;
				}

			if (of == IndexCount)
				Adjacency[f + edge] = f;
			else
				Adjacency[f + edge] = of / 3;
		}
	}
}


} // end namespace scene
} // end namespace daisy
