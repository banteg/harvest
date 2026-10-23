#include <iostream>
#include "ox/core/CPlane3d.h"
#include "ox/core/CString.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/SMesh.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "C3DSMeshFileLoader.h"
#include <string.h>
#include "daisy/os.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/SAnimatedMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


// Primary chunk
const unsigned short C3DS_MAIN3DS = 0x4D4D;

// Main Chunks
const unsigned short C3DS_EDIT3DS = 0x3D3D;
const unsigned short C3DS_KEYF3DS = 0xB000;
const unsigned short C3DS_VERSION = 0x0002;
const unsigned short C3DS_MESHVERSION = 0x3D3E;

// sub chunks of C3DS_EDIT3DS
const unsigned short C3DS_EDIT_MATERIAL = 0xAFFF;
const unsigned short C3DS_EDIT_OBJECT	= 0x4000;

// sub chunks of C3DS_EDIT_MATERIAL
const unsigned short C3DS_MATNAME       = 0xA000;
const unsigned short C3DS_MATLUMINANCE  = 0xA010;
const unsigned short C3DS_MATDIFFUSE    = 0xA020;
const unsigned short C3DS_MATSPECULAR   = 0xA030;
const unsigned short C3DS_MATSHININESS  = 0xA040;
const unsigned short C3DS_MATMAP        = 0xA200;
const unsigned short C3DS_MATREFLMAP    = 0xA220;
const unsigned short C3DS_MATOPACMAP    = 0xA210;
const unsigned short C3DS_MATSPECMAP    = 0xA204;
const unsigned short C3DS_MATBUMPMAP    = 0xA230;
const unsigned short C3DS_MATMAPFILE    = 0xA300;

// subs of C3DS_EDIT_OBJECT
const unsigned short C3DS_OBJTRIMESH    = 0x4100;

// subs of C3DS_OBJTRIMESH
const unsigned short C3DS_TRIVERT       = 0x4110;
const unsigned short C3DS_TRIFACE       = 0x4120;
const unsigned short C3DS_TRIFACEMAT    = 0x4130;
const unsigned short C3DS_TRIUV         = 0x4140;
const unsigned short C3DS_TRISMOOTH     = 0x4150;
const unsigned short C3DS_TRILOCAL      = 0x4160;

// different color chunk types
const unsigned short C3DS_COL_RGB  = 0x0010;
const unsigned short C3DS_COL_TRU  = 0x0011;
const unsigned short C3DS_COL_UNK  = 0x0013;


//! Constructor
C3DSMeshFileLoader::C3DSMeshFileLoader(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver)
: FileSystem(fs), Driver(driver), Vertices(0), CountVertices(0), CountIndices(0),
Indices(0), TCoords(0), CountTCoords(0), Mesh(0)
{
	if (FileSystem)
		FileSystem->grab();

	if (Driver)
		Driver->grab();
}



//! destructor
C3DSMeshFileLoader::~C3DSMeshFileLoader()
{
	if (FileSystem)
		FileSystem->drop();

	if (Driver)
		Driver->drop();

	if (Vertices)
		delete [] Vertices;

	if (Indices)
		delete [] Indices;

	if (TCoords)
		delete [] TCoords;

	if (Mesh)
		Mesh->drop();
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool C3DSMeshFileLoader::isALoadableFileExtension(const char* filename)
{
	return strstr(filename, ".3ds")!=0;
}



//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See ox::IUnknown::drop() for more information.
IAnimatedMesh* C3DSMeshFileLoader::createMesh(ox::io::IReadFile* file)
{
	ChunkData data;

	file->seek(0);

	file->read(&data.header, sizeof(ChunkHeader));
	if (data.header.id != 0x4D4D)
		return false;

	CurrentMaterial.Material = ox::video::SMaterial();
	CurrentMaterial.Name = "";
	CurrentMaterial.Filename = "";
	Materials.clear();
	cleanUp();

	if (Mesh)
		Mesh->drop();

	Mesh = new SMesh();

	if (readChunk(file, &data))
	{
		// success
		SAnimatedMesh* am = new SAnimatedMesh();
		am->Type = EAMT_3DS;

		for (int i=0; i<Mesh->getMeshBufferCount(); ++i)
			((SMeshBuffer*)Mesh->getMeshBuffer(i))->recalculateBoundingBox();

		Mesh->recalculateBoundingBox();

		am->addMesh(Mesh);
		am->recalculateBoundingBox();
		Mesh->drop();
		Mesh = 0;
        return am;
	}

	Mesh->drop();
	Mesh = 0;

    return 0;
}


bool C3DSMeshFileLoader::readColorChunk(ox::io::IReadFile* file, ChunkData* chunk,
										ox::video::SColor& out)
{
	ChunkData data;
	readChunkData(file, data);

	unsigned char c[3];

	if (sizeof(c) != data.header.length - data.read)
	{
		os::Printer::log("Unknown size of color chunk in 3Ds file.", ox::event::ELL_WARNING);
		file->seek(data.header.length - data.read, true);
		data.read += data.header.length - data.read;
	}
	else
	{
		file->read(c, data.header.length - data.read);
		out = ox::video::SColor(255, c[0], c[1], c[2]);
		data.read += data.header.length - data.read;
	}

	chunk->read += data.read;

	return true;
}


void C3DSMeshFileLoader::readMaterialGroup(ox::io::IReadFile* file, ChunkData& data)
{
	SMaterialGroup group;

	readString(file, data, group.MaterialName);

	file->read(&group.faceCount, sizeof(group.faceCount));
	data.read += sizeof(group.faceCount);

	// read faces
	group.faces = new short[group.faceCount];
	file->read(group.faces, sizeof(short) * group.faceCount);
	data.read += sizeof(short) * group.faceCount;

	MaterialGroups.push_back(group);
}


bool C3DSMeshFileLoader::readMaterialChunk(ox::io::IReadFile* file, ChunkData* parent)
{
	bool inSubChunk = false;

	while(parent->read < parent->header.length)
	{
		ChunkData data;
		readChunkData(file, data);

		switch(data.header.id)
		{
		case C3DS_MATNAME:
			{
				char* c = new char[data.header.length - data.read];
				file->read(c, data.header.length - data.read);

				if (strlen(c))
					CurrentMaterial.Name = c;

				data.read += data.header.length - data.read;
				delete [] c;
			}
			break;
		case C3DS_MATLUMINANCE:
			readColorChunk(file, &data, CurrentMaterial.Material.EmissiveColor);
			break;
		case C3DS_MATDIFFUSE:
			readColorChunk(file, &data, CurrentMaterial.Material.DiffuseColor);
			break;
		case C3DS_MATSPECULAR:
			readColorChunk(file, &data, CurrentMaterial.Material.SpecularColor);
			break;
		case C3DS_MATREFLMAP:
		case C3DS_MATMAP:
		case C3DS_MATOPACMAP:
		case C3DS_MATSPECMAP:
		case C3DS_MATBUMPMAP:
			readMaterialChunk(file, &data);
			break;
		case C3DS_MATMAPFILE:
			{
				// read texture file name
				char* c = new char[data.header.length - data.read];
				file->read(c, data.header.length - data.read);
				CurrentMaterial.Filename = c;
				data.read += data.header.length - data.read;
				delete [] c;
				inSubChunk = true;
			}
			break;
		case C3DS_MATSHININESS:
				// TODO: read shinniess
		default:
			// ignore chunk
			file->seek(data.header.length - data.read, true);
			data.read += data.header.length - data.read;
		}

		parent->read += data.read;
	}

	if (!inSubChunk)
		Materials.push_back(CurrentMaterial);

	return true;
}



bool C3DSMeshFileLoader::readChunk(ox::io::IReadFile* file, ChunkData* parent)
{
	while(parent->read < parent->header.length)
	{
		ChunkData data;
		readChunkData(file, data);

		switch(data.header.id)
		{
		case C3DS_VERSION:
			{
				unsigned short version;
				file->read(&version, sizeof(unsigned short));
				file->seek(data.header.length - data.read - 2, true);
				data.read += data.header.length - data.read;
				if (version != 0x03)
				{
					os::Printer::log("Cannot load 3ds files of version other than 3.", ox::event::ELL_ERROR);
					return false;
				}
			}
			break;
		case C3DS_EDIT_MATERIAL:
			readMaterialChunk(file, &data);
			break;
		case C3DS_EDIT3DS:
			{
				ChunkData ndata;
				readChunkData(file, ndata);
				if (ndata.header.id != C3DS_MESHVERSION)
				{
					os::Printer::log("Mesh version not where expected in 3ds file.", ox::event::ELL_ERROR);
					return false;
				}

				unsigned short version;
				file->read(&version, sizeof(unsigned short));
				file->seek(ndata.header.length - ndata.read - 2, true);
				data.read += ndata.read;

				/*if (version != 0x03)
				{
					os::Printer::log("Cannot read 3ds meshes of versions other than 3.", ox::event::ELL_ERROR);
					return false;
				}*/

				readChunk(file, &data);
			}
			break;

		case C3DS_EDIT_OBJECT:
			{
				readAndIgnoreString(file, data);
				readObjectChunk(file, &data);
			}
			break;

		default:
			// ignore chunk
			file->seek(data.header.length - data.read, true);
			data.read += data.header.length - data.read;
		}

		parent->read += data.read;
	}

	return true;
}



bool C3DSMeshFileLoader::readObjectChunk(ox::io::IReadFile* file, ChunkData* parent)
{
	while(parent->read < parent->header.length)
	{
		ChunkData data;
		readChunkData(file, data);

		switch(data.header.id)
		{
		case C3DS_OBJTRIMESH:
			readObjectChunk(file, &data);
			composeObject(file);
			break;

		case C3DS_TRIVERT:
			readVertices(file, data);
			break;

		case C3DS_TRIFACE:
			readIndices(file, data);
			readObjectChunk(file, &data); // read smooth and material groups
			break;

		case C3DS_TRIFACEMAT:
			readMaterialGroup(file, data);
			break;

		case C3DS_TRIUV: // geting texture coordiantes
			readTextureCoords(file, data);
			break;

		case C3DS_TRISMOOTH: // TODO
		case C3DS_TRILOCAL: // TODO
		default:
			// ignore chunk
			file->seek(data.header.length - data.read, true);
			data.read += data.header.length - data.read;
		}

		parent->read += data.read;
	}

	return true;
}


void C3DSMeshFileLoader::composeObject(ox::io::IReadFile* file)
{
	if (Mesh->getMeshBufferCount() != (int)Materials.size())
		loadMaterials(file);

	if (MaterialGroups.empty())
	{
		// no material group, so add all
		SMaterialGroup group;
		group.faceCount = CountIndices / 3;
		group.faces = new short[group.faceCount];
		for (int i=0; i<group.faceCount; ++i)
			group.faces[i] = i;
		MaterialGroups.push_back(group);

		// if we've got no material, add one without a texture
		if (Materials.empty())
		{
			SCurrentMaterial m;
			Materials.push_back(m);
			SMeshBuffer* mb = new ox::scene::SMeshBuffer();
			Mesh->addMeshBuffer(mb);
			mb->Material = Materials[0].Material;
			mb->drop();
		}
	}

	for (unsigned int i=0; i<MaterialGroups.size(); ++i)
	{
		SMeshBuffer* mb = 0;

		// find mesh buffer for this group
		for (unsigned int j=0; j<Materials.size(); ++j)
			if (MaterialGroups[i].MaterialName == Materials[j].Name)
			{
				mb = (SMeshBuffer*)Mesh->getMeshBuffer(j);
				break;
			}


		if (mb != 0)
		{
			// add geometry to the buffer.

			ox::video::S3DVertex vtx;
			vtx.Color = ox::video::SColor(255, 255, 255, 255);
			vtx.Normal.set(0,0,0);

			for (int f=0; f<MaterialGroups[i].faceCount; ++f)
			{
				int vtxCount = mb->Vertices.size();

				for (int v=0; v<3; ++v)
				{
					int idx = Indices[MaterialGroups[i].faces[f]*4 +v];

					if (CountVertices > idx)
					{
						vtx.Pos.X = Vertices[idx*3 + 0];
						vtx.Pos.Z = Vertices[idx*3 + 1];
						vtx.Pos.Y = Vertices[idx*3 + 2];
					}

					if (CountTCoords > idx)
					{
						vtx.TCoords.X = TCoords[idx*2 + 0];
						vtx.TCoords.Y = 1.0f -TCoords[idx*2 + 1];
					}

					mb->Vertices.push_back(vtx);
				}

				// compute normal
				ox::core::CPlane3d<float> pl(mb->Vertices[vtxCount].Pos, mb->Vertices[vtxCount+2].Pos,
						mb->Vertices[vtxCount+1].Pos);

				mb->Vertices[vtxCount].Normal = pl.Normal;
				mb->Vertices[vtxCount+1].Normal = pl.Normal;
				mb->Vertices[vtxCount+2].Normal = pl.Normal;

				// add indices

				mb->Indices.push_back(vtxCount);
				mb->Indices.push_back(vtxCount+2);
				mb->Indices.push_back(vtxCount+1);
			}
		}
		else
			os::Printer::log("Found no matching material for Group in 3ds file.", ox::event::ELL_WARNING);
	}

	cleanUp();
}


ox::core::CString<char> C3DSMeshFileLoader::getTextureFileName(ox::core::CString<char> texture,
													 ox::core::CString<char> model)
{
	int idx = -1;
	idx = model.findLast('/');

    if (idx == -1)
		idx = model.findLast('\\');

	if (idx == -1)
		return ox::core::CString<char>();

	ox::core::CString<char> p = model.subString(0, idx+1);
	p.append(texture);
	return p;
}


void C3DSMeshFileLoader::loadMaterials(ox::io::IReadFile* file)
{
	// create a mesh buffer for every material

	ox::core::CString<char> modelFilename = file->getFileName();

	if (Materials.empty())
		os::Printer::log("No materials found in 3ds file.", ox::event::ELL_INFORMATION);

	for (unsigned int i=0; i<Materials.size(); ++i)
	{
		SMeshBuffer* m = new ox::scene::SMeshBuffer();
		Mesh->addMeshBuffer(m);

		m->Material = Materials[i].Material;
		if (Materials[i].Filename.size())
		{
			ox::core::CString<char> fname = getTextureFileName(
				Materials[i].Filename, modelFilename);

			m->Material.Texture1 = Driver->getTexture(Materials[i].Filename.c_str());

			if (!m->Material.Texture1)
				m->Material.Texture1 = Driver->getTexture(fname.c_str());

			if (!m->Material.Texture1)
				os::Printer::log("Could not find a texture for entry in 3ds file",
					Materials[i].Filename.c_str(), ox::event::ELL_WARNING);
		}

		m->drop();
	}
}


void C3DSMeshFileLoader::cleanUp()
{
	delete [] Vertices;
	CountVertices = 0;
	Vertices = 0;
	delete [] Indices;
	Indices = 0;
	CountIndices = 0;
	delete [] TCoords;
	TCoords = 0;
	CountTCoords = 0;

	MaterialGroups.clear();
}




void C3DSMeshFileLoader::readTextureCoords(ox::io::IReadFile* file, ChunkData& data)
{
	file->read(&CountTCoords, sizeof(CountTCoords));
	data.read += sizeof(CountTCoords);

	int tcoordsBufferByteSize = CountTCoords * sizeof(float) * 2;

	if (data.header.length - data.read != tcoordsBufferByteSize)
	{
		os::Printer::log("Invalid size of tcoords found in 3ds file.", ox::event::ELL_WARNING);
		return;
	}

	TCoords = new float[CountTCoords * 3];
	file->read(TCoords, tcoordsBufferByteSize);
	data.read += tcoordsBufferByteSize;
}


void C3DSMeshFileLoader::readIndices(ox::io::IReadFile* file, ChunkData& data)
{
	file->read(&CountIndices, sizeof(CountIndices));
	data.read += sizeof(CountIndices);

	int indexBufferByteSize = CountIndices * sizeof(unsigned short) * 4;

	// Indices are u16s.
	// After every 3 Indices in the array, there follows an edge flag.
	Indices = new unsigned short[CountIndices * 4];
	file->read(Indices, indexBufferByteSize);
	data.read += indexBufferByteSize;
}



void C3DSMeshFileLoader::readVertices(ox::io::IReadFile* file, ChunkData& data)
{
	file->read(&CountVertices, sizeof(CountVertices));
	data.read += sizeof(CountVertices);

	int vertexBufferByteSize = CountVertices * sizeof(float) * 3;

	if (data.header.length - data.read != vertexBufferByteSize)
	{
		os::Printer::log("Invalid size of vertices found in 3ds file.", ox::event::ELL_WARNING);
		return;
	}

	Vertices = new float[CountVertices * 3];
	file->read(Vertices, vertexBufferByteSize);
	data.read += vertexBufferByteSize;
}



void C3DSMeshFileLoader::readChunkData(ox::io::IReadFile* file, ChunkData& data)
{
	file->read(&data.header, sizeof(ChunkHeader));
	data.read += sizeof(ChunkHeader);
}


void C3DSMeshFileLoader::readAndIgnoreString(ox::io::IReadFile* file, ChunkData& data)
{
	char c = 1;

	while (c)
	{
		if (!file->read(&c, sizeof(char)))
			return;

		++data.read;
	}
}

void C3DSMeshFileLoader::readString(ox::io::IReadFile* file, ChunkData& data, ox::core::CString<char>& out)
{
	char c = 1;
	out = "";

	while (c)
	{
		file->read(&c, sizeof(char));
		if (c)
			out.append(c);

		++data.read;
	}
}


} // end namespace scene
} // end namespace daisy
