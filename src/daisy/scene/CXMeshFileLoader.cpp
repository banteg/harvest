#include <iostream>
#include "ox/video/IVideoDriver.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/io/IReadFile.h"
#include "CXMeshFileLoader.h"
#include <string.h>
#include "daisy/os.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/SAnimatedMesh.h"
#include "CXFileReader.h"
#include "CXAnimationPlayer.h"
#include "ox/scene/IMeshManipulator.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Constructor
CXMeshFileLoader::CXMeshFileLoader(IMeshManipulator* manip,
								   ox::video::IVideoDriver* driver)
: Manipulator(manip), Driver(driver)
{
	if (Manipulator)
		Manipulator->grab();

	if (Driver)
		Driver->grab();
}



//! destructor
CXMeshFileLoader::~CXMeshFileLoader()
{
	if (Manipulator)
		Manipulator->drop();

	if (Driver)
		Driver->drop();
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CXMeshFileLoader::isALoadableFileExtension(const char* filename)
{
	return strstr(filename, ".x")!=0;
}



//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See ox::IUnknown::drop() for more information.
IAnimatedMesh* CXMeshFileLoader::createMesh(ox::io::IReadFile* file)
{
	if (!file)
		return 0;

	IAnimatedMesh* mesh = 0;
	CXFileReader *xreader = new CXFileReader(file);

	if (xreader->errorHappened())
	{
		xreader->drop();
		return 0;
	}

	mesh = new CXAnimationPlayer(xreader, Driver, Manipulator,
		file->getFileName());

	xreader->drop();
	return mesh;
}


} // end namespace scene
} // end namespace daisy
