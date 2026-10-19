// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CQ3LevelMesh.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/SMaterial.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SColor.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "ox/IUnknown.h"
#include "ox/TArray.h"
#include "CQ3LevelMesh.h"
#include "ox/scene/ISceneManager.h"
#include "daisy/os.h"
#include "ox/scene/SMeshBufferLightMap.h"
#include "ox/core/CString.h"
#include "daisy/video/Null/CColorConverter.h"
#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CQ3LevelMesh::CQ3LevelMesh(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver)
: Textures(0), LightMaps(0),
 Vertices(0), Faces(0),	Planes(0), Nodes(0), Leafs(0), LeafFaces(0),
	MeshVerts(0), Brushes(0), Driver(driver), FileSystem(fs)
{
	#ifdef _DEBUG
	ox::IUnknown::setDebugName("CQ3LevelMesh");
	#endif	

	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();
}


//! destructor
CQ3LevelMesh::~CQ3LevelMesh()
{
	if (Textures)
		delete [] Textures;

	if (LightMaps)
		delete [] LightMaps;
	
	if (Vertices)
		delete [] Vertices;

	if (Faces)
		delete [] Faces;

	if (Planes)
		delete [] Planes;

	if (Nodes)
		delete [] Nodes;

	if (Leafs)
		delete [] Leafs;

	if (LeafFaces)
		delete [] LeafFaces;
	
	if (MeshVerts)
		delete [] MeshVerts;

	if (Brushes)
		delete [] Brushes;

	if (Driver)
		Driver->drop();

	if (FileSystem)
		FileSystem->drop();
}



//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
bool CQ3LevelMesh::loadFile(ox::io::IReadFile* file)
{
	if (!file)
		return false;

	LevelName = file->getFileName();

	tBSPHeader header;
	file->read(&header, sizeof(tBSPHeader));

	if (header.strID != 0x50534249 || header.version != 0x2e)
	{
		os::Printer::log("Could not load .bsp file, unknown header.", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	// now read lumps

	file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);

	// load everything

	loadTextures(&Lumps[kTextures], file);		// Load the textures
	loadLightmaps(&Lumps[kLightmaps], file);      // Load the lightmaps
	loadVerts(&Lumps[kVertices], file);		// Load the vertices
	loadFaces(&Lumps[kFaces], file);		// Load the faces
	loadPlanes(&Lumps[kPlanes], file);		// Load the Planes of the BSP
	loadNodes(&Lumps[kNodes], file);		// load the Nodes of the BSP
	loadLeafs(&Lumps[kLeafs], file);		// load the Leafs of the BSP
	loadLeafFaces(&Lumps[kLeafFaces], file);		// load the Faces of the Leafs of the BSP
	loadVisData(&Lumps[kVisData], file);		// load the visibility data of the clusters
	loadEntities(&Lumps[kEntities], file);		// load the entities
	loadModels(&Lumps[kModels], file);		// load the models
	loadMeshVerts(&Lumps[kMeshVerts], file);		// load the mesh vertices
	loadBrushes(&Lumps[kBrushes], file);		// load the brushes of the BSP
	loadBrushSides(&Lumps[kBrushSides], file);		// load the brushsides of the BSP
	loadLeafBrushes(&Lumps[kLeafBrushes], file);		// load the brushes of the leaf*/

	constructMesh();

	loadTextures();

	return true;
}



//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
int CQ3LevelMesh::getFrameCount()
{
	return 1;
}



//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CQ3LevelMesh::getMesh(int frameInMs, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	return &Mesh;
}


void CQ3LevelMesh::loadTextures(tBSPLump* l, ox::io::IReadFile* file)
{
	NumTextures = l->length / sizeof(tBSPTexture);
	Textures = new tBSPTexture[NumTextures];

	file->seek(l->offset);
	file->read(Textures, l->length);
}


void CQ3LevelMesh::loadLightmaps(tBSPLump* l, ox::io::IReadFile* file)
{
	NumLightMaps = l->length / sizeof(tBSPLightmap);
	LightMaps = new tBSPLightmap[NumLightMaps];

	file->seek(l->offset);
	file->read(LightMaps, l->length);
}


void CQ3LevelMesh::loadVerts(tBSPLump* l, ox::io::IReadFile* file)
{
	NumVertices = l->length / sizeof(tBSPVertex);
	Vertices = new tBSPVertex[NumVertices];

	file->seek(l->offset);
	file->read(Vertices, l->length);
}


void CQ3LevelMesh::loadFaces(tBSPLump* l, ox::io::IReadFile* file)
{
	NumFaces = l->length / sizeof(tBSPFace);
	Faces = new tBSPFace[NumFaces];

	file->seek(l->offset);
	file->read(Faces, l->length);
}


void CQ3LevelMesh::loadPlanes(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadNodes(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadLeafs(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadLeafFaces(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadVisData(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadEntities(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadModels(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadMeshVerts(tBSPLump* l, ox::io::IReadFile* file)
{
	NumMeshVerts = l->length / sizeof(int);
	MeshVerts = new int[NumMeshVerts];

	file->seek(l->offset);
	file->read(MeshVerts, l->length);
}


void CQ3LevelMesh::loadBrushes(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadBrushSides(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


void CQ3LevelMesh::loadLeafBrushes(tBSPLump* l, ox::io::IReadFile* file)
{
	// ignore
}


//! constructs a mesh from the quake 3 level file.
void CQ3LevelMesh::constructMesh()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}

// helper method for creating curved surfaces, sent in by Dean P. Macri.
inline float CQ3LevelMesh::Blend( float s[3], float t[3], tBSPVertex *v[9], int offset)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


void CQ3LevelMesh::createCurvedSurface(SMeshBufferLightMap* meshBuffer, int i)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! loads the textures
void CQ3LevelMesh::loadTextures()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! Returns an axis aligned bounding box of the mesh.
//! \return ox::core::A bounding box of this mesh is returned.
const ox::core::CAabbox3d<float>& CQ3LevelMesh::getBoundingBox() const
{
	return Mesh.getBoundingBox();
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CQ3LevelMesh::getMeshType() const
{
	return ox::scene::EAMT_BSP;
}

} // end namespace scene
} // end namespace daisy

