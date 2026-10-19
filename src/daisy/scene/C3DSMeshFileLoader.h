#ifndef __C_3DS_MESH_FILE_LOADER_H_INCLUDED__
#define __C_3DS_MESH_FILE_LOADER_H_INCLUDED__

#include "ox/scene/IMeshLoader.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/io/IReadFile.h"
#include "ox/TArray.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IVideoDriver.h"
#include "ox/core/CString.h"
#include "ox/scene/SMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Meshloader capable of loading 3ds meshes.
class C3DSMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	C3DSMeshFileLoader(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver);

	//! destructor
	virtual ~C3DSMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See ox::IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(ox::io::IReadFile* file);

private:

	// byte-align structures
	#ifdef _MSC_VER
	#	pragma pack( push, packing )
	#	pragma pack( 1 )
	#	define PACK_STRUCT
	#elif defined( __GNUC__ )
	#	define PACK_STRUCT	__attribute__((packed))
	#else
	#	error compiler not supported
	#endif

	struct ChunkHeader
	{
		unsigned short id;
		int length;
	} PACK_STRUCT;

	// Default alignment
	#ifdef _MSC_VER
	#	pragma pack( pop, packing )
	#endif

	#undef PACK_STRUCT


	struct ChunkData
	{
		ChunkData() : read(0) {}

		ChunkHeader header;
		int read;
	};

	struct SCurrentMaterial
	{
		SCurrentMaterial() {};
		SCurrentMaterial(const SCurrentMaterial& o) : Material(o.Material) {};

		~SCurrentMaterial() { 	};

		ox::video::SMaterial Material;
		ox::core::CString<char> Name;
		ox::core::CString<char> Filename;
	};

	struct SMaterialGroup
	{
		SMaterialGroup() : faceCount(0), faces(0) {};

		SMaterialGroup(const SMaterialGroup& o)
		{
			*this = o;
		}

		~SMaterialGroup() 
		{ 
			clear();
		}

		void clear() 
		{ 
			if (faces) delete [] faces;
			faces = 0;
			faceCount = 0;
		}

		void operator =(const SMaterialGroup& o)
		{
			MaterialName = o.MaterialName;
			faceCount = o.faceCount;
			faces = new short[faceCount];
			for (int i=0; i<faceCount; ++i)
				faces[i] = o.faces[i];
		}

		ox::core::CString<char> MaterialName;
		short faceCount;
		short* faces;
	};

	bool readChunk(ox::io::IReadFile* file, ChunkData* parent);
	bool readMaterialChunk(ox::io::IReadFile* file, ChunkData* parent);
	bool readObjectChunk(ox::io::IReadFile* file, ChunkData* parent);
	bool readColorChunk(ox::io::IReadFile* file, ChunkData* chunk, ox::video::SColor& out);

	void readChunkData(ox::io::IReadFile* file, ChunkData& data);
	void readAndIgnoreString(ox::io::IReadFile* file, ChunkData& data);
	void readString(ox::io::IReadFile* file, ChunkData& data, ox::core::CString<char>& out);
	void readVertices(ox::io::IReadFile* file, ChunkData& data);
	void readIndices(ox::io::IReadFile* file, ChunkData& data);
	void readMaterialGroup(ox::io::IReadFile* file, ChunkData& data);
	void readTextureCoords(ox::io::IReadFile* file, ChunkData& data);

	void composeObject(ox::io::IReadFile* file);
	void loadMaterials(ox::io::IReadFile* file);
	void cleanUp();
	ox::core::CString<char> getTextureFileName(ox::core::CString<char> texture, ox::core::CString<char> model);

	ox::io::IFileSystem* FileSystem;
	ox::video::IVideoDriver* Driver;

	float* Vertices;
	short CountVertices;
	unsigned short* Indices;
	short CountIndices;
	short CountTCoords;
	float* TCoords;
	ox::TArray<SMaterialGroup> MaterialGroups;

	SCurrentMaterial CurrentMaterial;
	ox::TArray<SCurrentMaterial> Materials;

	SMesh* Mesh;
};

} // end namespace scene
} // end namespace daisy

#endif

