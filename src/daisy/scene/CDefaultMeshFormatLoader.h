#ifndef __C_DEFAULT_MESH_FORMAT_LOADER_H_INCLUDED__
#define __C_DEFAULT_MESH_FORMAT_LOADER_H_INCLUDED__

#include "ox/scene/IMeshLoader.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IVideoDriver.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Meshloader capable of loading all Irrlicht default build in formats.
/** Which are: Quake 3 Bsp level, Quake 2 MD2 model, Maya .obj mesh,
 Milkshape .ms3d model. */
class CDefaultMeshFormatLoader : public IMeshLoader
{
public:

	//! Constructor
	CDefaultMeshFormatLoader(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver);

	//! destructor
	virtual ~CDefaultMeshFormatLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".bsp")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See ox::IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(ox::io::IReadFile* file);

private:

	ox::io::IFileSystem* FileSystem;
	ox::video::IVideoDriver* Driver;
};

} // end namespace scene
} // end namespace daisy

#endif
