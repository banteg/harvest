#ifndef __C_X_MESH_FILE_LOADER_H_INCLUDED__
#define __C_X_MESH_FILE_LOADER_H_INCLUDED__

#include "ox/scene/IMeshLoader.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMeshManipulator.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IVideoDriver.h"
#include "ox/core/CString.h"
#include "ox/scene/SMesh.h"
#include "CXFileReader.h"
#include "ox/scene/SMeshBuffer.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Meshloader capable of loading x meshes.
class CXMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CXMeshFileLoader(IMeshManipulator* manip, ox::video::IVideoDriver* driver);

	//! destructor
	virtual ~CXMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See ox::IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(ox::io::IReadFile* file);

private:

	IMeshManipulator* Manipulator;
	ox::video::IVideoDriver* Driver;
};

} // end namespace scene
} // end namespace daisy

#endif

