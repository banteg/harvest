// Copyright (C) 2002-2003 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMeshLoader.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_MESH_LOADER_H_INCLUDED__
#define __I_MESH_LOADER_H_INCLUDED__

#include "ox/IUnknown.h"
#include "ox/io/IReadFile.h"
#include "IAnimatedMesh.h"

namespace ox
{
namespace scene
{

//!	Class which is able to load an animated mesh from a file.
/** If you want the Irrlicht Engine be able to load meshes of
currently unsupported file formats (e.g .cob), then implement
this and add your new Surface loader with
ISceneManager::addExternalMeshLoader() to the engine. */
class IMeshLoader : public ox::IUnknown
{
public:

	//! destructor
	virtual ~IMeshLoader() {};

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".bsp")
	virtual bool isALoadableFileExtension(const char* fileName) = 0;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See ox::IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(ox::io::IReadFile* file) = 0;
};


} // end namespace scene
} // end namespace ox

#endif
