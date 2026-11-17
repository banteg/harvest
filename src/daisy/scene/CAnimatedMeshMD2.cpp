// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshMD2.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshMD2.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CAnimatedMeshMD2.h"
#include "daisy/os.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"
#include "ox/io/IReadFile.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

#ifdef _MSC_VER
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error compiler not supported
#endif

	// structs needed to load the md2-format

	const int MD2_MAGIC_NUMBER = 844121161;
	const int MD2_VERSION		= 8;
	const int MD2_MAX_VERTS		= 2048;
	const int MD2_FRAME_SHIFT = 3;

	struct SMD2Header
	{
		int magic;
		int version;
		int skinWidth;
		int skinHeight;
		int frameSize;
		int numSkins;
		int numVertices;
		int numTexcoords;
		int numTriangles;
		int numGlCommands;
		int numFrames;
		int offsetSkins;
		int offsetTexcoords;
		int offsetTriangles;
		int offsetFrames;
		int offsetGlCommands;
		int offsetEnd;
	} PACK_STRUCT;

	struct SMD2Vertex
	{
		unsigned char vertex[3];
		unsigned char lightNormalIndex;
	} PACK_STRUCT;

	struct SMD2Frame
	{
		float	scale[3];
		float	translate[3];
		char	name[16];
		SMD2Vertex vertices[1];
	} PACK_STRUCT;

	struct SMD2Triangle
	{
		short vertexIndices[3];
		short textureIndices[3];
	} PACK_STRUCT;

	struct SMD2TextureCoordinate
	{
		short s;
		short t;
	} PACK_STRUCT;

	struct SMD2GLCommand
	{
		float s, t;
		int vertexIndex;
	} PACK_STRUCT;

// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT


const int Q2_VERTEX_NORMAL_TABLE_SIZE = 162;

const float Q2_VERTEX_NORMAL_TABLE[Q2_VERTEX_NORMAL_TABLE_SIZE][3] = {
	{-0.525731f, 0.000000f, 0.850651f},
	{-0.442863f, 0.238856f, 0.864188f},
	{-0.295242f, 0.000000f, 0.955423f},
	{-0.309017f, 0.500000f, 0.809017f},
	{-0.162460f, 0.262866f, 0.951056f},
	{0.000000f, 0.000000f, 1.000000f},
	{0.000000f, 0.850651f, 0.525731f},
	{-0.147621f, 0.716567f, 0.681718f},
	{0.147621f, 0.716567f, 0.681718f},
	{0.000000f, 0.525731f, 0.850651f},
	{0.309017f, 0.500000f, 0.809017f},
	{0.525731f, 0.000000f, 0.850651f},
	{0.295242f, 0.000000f, 0.955423f},
	{0.442863f, 0.238856f, 0.864188f},
	{0.162460f, 0.262866f, 0.951056f},
	{-0.681718f, 0.147621f, 0.716567f},
	{-0.809017f, 0.309017f, 0.500000f},
	{-0.587785f, 0.425325f, 0.688191f},
	{-0.850651f, 0.525731f, 0.000000f},
	{-0.864188f, 0.442863f, 0.238856f},
	{-0.716567f, 0.681718f, 0.147621f},
	{-0.688191f, 0.587785f, 0.425325f},
	{-0.500000f, 0.809017f, 0.309017f},
	{-0.238856f, 0.864188f, 0.442863f},
	{-0.425325f, 0.688191f, 0.587785f},
	{-0.716567f, 0.681718f, -0.147621f},
	{-0.500000f, 0.809017f, -0.309017f},
	{-0.525731f, 0.850651f, 0.000000f},
	{0.000000f, 0.850651f, -0.525731f},
	{-0.238856f, 0.864188f, -0.442863f},
	{0.000000f, 0.955423f, -0.295242f},
	{-0.262866f, 0.951056f, -0.162460f},
	{0.000000f, 1.000000f, 0.000000f},
	{0.000000f, 0.955423f, 0.295242f},
	{-0.262866f, 0.951056f, 0.162460f},
	{0.238856f, 0.864188f, 0.442863f},
	{0.262866f, 0.951056f, 0.162460f},
	{0.500000f, 0.809017f, 0.309017f},
	{0.238856f, 0.864188f, -0.442863f},
	{0.262866f, 0.951056f, -0.162460f},
	{0.500000f, 0.809017f, -0.309017f},
	{0.850651f, 0.525731f, 0.000000f},
	{0.716567f, 0.681718f, 0.147621f},
	{0.716567f, 0.681718f, -0.147621f},
	{0.525731f, 0.850651f, 0.000000f},
	{0.425325f, 0.688191f, 0.587785f},
	{0.864188f, 0.442863f, 0.238856f},
	{0.688191f, 0.587785f, 0.425325f},
	{0.809017f, 0.309017f, 0.500000f},
	{0.681718f, 0.147621f, 0.716567f},
	{0.587785f, 0.425325f, 0.688191f},
	{0.955423f, 0.295242f, 0.000000f},
	{1.000000f, 0.000000f, 0.000000f},
	{0.951056f, 0.162460f, 0.262866f},
	{0.850651f, -0.525731f, 0.000000f},
	{0.955423f, -0.295242f, 0.000000f},
	{0.864188f, -0.442863f, 0.238856f},
	{0.951056f, -0.162460f, 0.262866f},
	{0.809017f, -0.309017f, 0.500000f},
	{0.681718f, -0.147621f, 0.716567f},
	{0.850651f, 0.000000f, 0.525731f},
	{0.864188f, 0.442863f, -0.238856f},
	{0.809017f, 0.309017f, -0.500000f},
	{0.951056f, 0.162460f, -0.262866f},
	{0.525731f, 0.000000f, -0.850651f},
	{0.681718f, 0.147621f, -0.716567f},
	{0.681718f, -0.147621f, -0.716567f},
	{0.850651f, 0.000000f, -0.525731f},
	{0.809017f, -0.309017f, -0.500000f},
	{0.864188f, -0.442863f, -0.238856f},
	{0.951056f, -0.162460f, -0.262866f},
	{0.147621f, 0.716567f, -0.681718f},
	{0.309017f, 0.500000f, -0.809017f},
	{0.425325f, 0.688191f, -0.587785f},
	{0.442863f, 0.238856f, -0.864188f},
	{0.587785f, 0.425325f, -0.688191f},
	{0.688191f, 0.587785f, -0.425325f},
	{-0.147621f, 0.716567f, -0.681718f},
	{-0.309017f, 0.500000f, -0.809017f},
	{0.000000f, 0.525731f, -0.850651f},
	{-0.525731f, 0.000000f, -0.850651f},
	{-0.442863f, 0.238856f, -0.864188f},
	{-0.295242f, 0.000000f, -0.955423f},
	{-0.162460f, 0.262866f, -0.951056f},
	{0.000000f, 0.000000f, -1.000000f},
	{0.295242f, 0.000000f, -0.955423f},
	{0.162460f, 0.262866f, -0.951056f},
	{-0.442863f, -0.238856f, -0.864188f},
	{-0.309017f, -0.500000f, -0.809017f},
	{-0.162460f, -0.262866f, -0.951056f},
	{0.000000f, -0.850651f, -0.525731f},
	{-0.147621f, -0.716567f, -0.681718f},
	{0.147621f, -0.716567f, -0.681718f},
	{0.000000f, -0.525731f, -0.850651f},
	{0.309017f, -0.500000f, -0.809017f},
	{0.442863f, -0.238856f, -0.864188f},
	{0.162460f, -0.262866f, -0.951056f},
	{0.238856f, -0.864188f, -0.442863f},
	{0.500000f, -0.809017f, -0.309017f},
	{0.425325f, -0.688191f, -0.587785f},
	{0.716567f, -0.681718f, -0.147621f},
	{0.688191f, -0.587785f, -0.425325f},
	{0.587785f, -0.425325f, -0.688191f},
	{0.000000f, -0.955423f, -0.295242f},
	{0.000000f, -1.000000f, 0.000000f},
	{0.262866f, -0.951056f, -0.162460f},
	{0.000000f, -0.850651f, 0.525731f},
	{0.000000f, -0.955423f, 0.295242f},
	{0.238856f, -0.864188f, 0.442863f},
	{0.262866f, -0.951056f, 0.162460f},
	{0.500000f, -0.809017f, 0.309017f},
	{0.716567f, -0.681718f, 0.147621f},
	{0.525731f, -0.850651f, 0.000000f},
	{-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f, -0.309017f},
	{-0.262866f, -0.951056f, -0.162460f},
	{-0.850651f, -0.525731f, 0.000000f},
	{-0.716567f, -0.681718f, -0.147621f},
	{-0.716567f, -0.681718f, 0.147621f},
	{-0.525731f, -0.850651f, 0.000000f},
	{-0.500000f, -0.809017f, 0.309017f},
	{-0.238856f, -0.864188f, 0.442863f},
	{-0.262866f, -0.951056f, 0.162460f},
	{-0.864188f, -0.442863f, 0.238856f},
	{-0.809017f, -0.309017f, 0.500000f},
	{-0.688191f, -0.587785f, 0.425325f},
	{-0.681718f, -0.147621f, 0.716567f},
	{-0.442863f, -0.238856f, 0.864188f},
	{-0.587785f, -0.425325f, 0.688191f},
	{-0.309017f, -0.500000f, 0.809017f},
	{-0.147621f, -0.716567f, 0.681718f},
	{-0.425325f, -0.688191f, 0.587785f},
	{-0.162460f, -0.262866f, 0.951056f},
	{0.442863f, -0.238856f, 0.864188f},
	{0.162460f, -0.262866f, 0.951056f},
	{0.309017f, -0.500000f, 0.809017f},
	{0.147621f, -0.716567f, 0.681718f},
	{0.000000f, -0.525731f, 0.850651f},
	{0.425325f, -0.688191f, 0.587785f},
	{0.587785f, -0.425325f, 0.688191f},
	{0.688191f, -0.587785f, 0.425325f},
	{-0.955423f, 0.295242f, 0.000000f},
	{-0.951056f, 0.162460f, 0.262866f},
	{-1.000000f, 0.000000f, 0.000000f},
	{-0.850651f, 0.000000f, 0.525731f},
	{-0.955423f, -0.295242f, 0.000000f},
	{-0.951056f, -0.162460f, 0.262866f},
	{-0.864188f, 0.442863f, -0.238856f},
	{-0.951056f, 0.162460f, -0.262866f},
	{-0.809017f, 0.309017f, -0.500000f},
	{-0.864188f, -0.442863f, -0.238856f},
	{-0.951056f, -0.162460f, -0.262866f},
	{-0.809017f, -0.309017f, -0.500000f},
	{-0.681718f, 0.147621f, -0.716567f},
	{-0.681718f, -0.147621f, -0.716567f},
	{-0.850651f, 0.000000f, -0.525731f},
	{-0.688191f, 0.587785f, -0.425325f},
	{-0.587785f, 0.425325f, -0.688191f},
	{-0.425325f, 0.688191f, -0.587785f},
	{-0.425325f, -0.688191f, -0.587785f},
	{-0.587785f, -0.425325f, -0.688191f},
	{-0.688191f, -0.587785f, -0.425325f},
	};

struct SMD2AnimationType
{
	int begin;
	int end;
	int fps;
};

SMD2AnimationType MD2AnimationTypeList[21] =
{
    {   0,  39,  9 },   // STAND
    {  40,  45, 10 },   // RUN
    {  46,  53, 10 },   // ATTACK
    {  54,  57,  7 },   // PAIN_A
    {  58,  61,  7 },   // PAIN_B
    {  62,  65,  7 },   // PAIN_C
    {  66,  71,  7 },   // JUMP
    {  72,  83,  7 },   // FLIP
    {  84,  94,  7 },   // SALUTE
    {  95, 111, 10 },   // FALLBACK
    { 112, 122,  7 },   // WAVE
    { 123, 134,  6 },   // POINT
    { 135, 153, 10 },   // CROUCH_STAND
    { 154, 159,  7 },   // CROUCH_WALK
    { 160, 168, 10 },   // CROUCH_ATTACK
    { 196, 172,  7 },   // CROUCH_PAIN
    { 173, 177,  5 },   // CROUCH_DEATH
    { 178, 183,  7 },   // DEATH_FALLBACK
    { 184, 189,  7 },   // DEATH_FALLFORWARD
    { 190, 197,  7 },   // DEATH_FALLBACKSLOW
    { 198, 198,  5 },   // BOOM
};


//! constructor
CAnimatedMeshMD2::CAnimatedMeshMD2()
: FrameCount(0), FrameList(0)
{
	#ifdef _DEBUG
	IAnimatedMesh::setDebugName("CAnimatedMeshMD2 IAnimatedMesh");
	IMesh::setDebugName("CAnimatedMeshMD2 IMesh");
	IMeshBuffer::setDebugName("CAnimatedMeshMD2 IMeshBuffer");
	#endif
}



//! destructor
CAnimatedMeshMD2::~CAnimatedMeshMD2()
{
	if (FrameList)
		delete [] FrameList;
}



//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
int CAnimatedMeshMD2::getFrameCount()
{
	return FrameCount<<MD2_FRAME_SHIFT;
}



//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CAnimatedMeshMD2::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	if ((unsigned int)frame > (FrameCount<<MD2_FRAME_SHIFT))
		frame = (frame % (FrameCount<<MD2_FRAME_SHIFT));

	if (startFrameLoop == -1 && endFrameLoop == -1)
	{
		startFrameLoop = 0;
		endFrameLoop = FrameCount<<MD2_FRAME_SHIFT;
	}

	updateInterpolationBuffer(frame, startFrameLoop, endFrameLoop);
	return this;
}


//! returns amount of mesh buffers.
int getMeshBufferCount()
{
	return 1;
}


//! returns pointer to a mesh buffer
IMeshBuffer* CAnimatedMeshMD2::getMeshBuffer(int nr)
{
	return this;
}



//! returns the material of this meshbuffer
const ox::video::SMaterial& CAnimatedMeshMD2::getMaterial() const
{
	return Material;
}


//! returns the material of this meshbuffer
ox::video::SMaterial& CAnimatedMeshMD2::getMaterial()
{
	return Material;
}



//! returns pointer to vertices
const void* CAnimatedMeshMD2::getVertices() const
{
	return &InterpolateBuffer[0];
}


//! returns pointer to vertices
void* CAnimatedMeshMD2::getVertices()
{
	return &InterpolateBuffer[0];
}



//! returns which type of vertex data is stored.
ox::video::E_VERTEX_TYPE CAnimatedMeshMD2::getVertexType() const
{
	return ox::video::EVT_STANDARD;
}



//! returns amount of vertices
int CAnimatedMeshMD2::getVertexCount() const
{
	return FrameList[0].size();
}



//! returns pointer to Indices
const unsigned short* CAnimatedMeshMD2::getIndices() const
{
	return &Indices[0];
}



//! returns pointer to Indices
unsigned short* CAnimatedMeshMD2::getIndices()
{
	return &Indices[0];
}



//! returns amount of indices
int CAnimatedMeshMD2::getIndexCount() const
{
	return Indices.size();
}


//! returns amount of mesh buffers.
int CAnimatedMeshMD2::getMeshBufferCount()
{
	return 1;
}


// updates the interpolation buffer
void CAnimatedMeshMD2::updateInterpolationBuffer(int frame, int startFrameLoop, int endFrameLoop)
{
	unsigned int firstFrame, secondFrame;
	float div;

	if (endFrameLoop - startFrameLoop == 0)
	{
		firstFrame = frame>>MD2_FRAME_SHIFT;
		secondFrame = frame>>MD2_FRAME_SHIFT;
		div = 1.0f;
	}
	else
	{
		firstFrame = frame>>MD2_FRAME_SHIFT;

		if (!((endFrameLoop>>MD2_FRAME_SHIFT) - (startFrameLoop>>MD2_FRAME_SHIFT)))
			secondFrame = firstFrame;
		else
		secondFrame = (startFrameLoop>>MD2_FRAME_SHIFT) + (((frame>>MD2_FRAME_SHIFT)+1 - (startFrameLoop>>MD2_FRAME_SHIFT)) %
			((endFrameLoop>>MD2_FRAME_SHIFT) - (startFrameLoop>>MD2_FRAME_SHIFT)));

		div = (frame % (1<<MD2_FRAME_SHIFT)) / (float)(1<<MD2_FRAME_SHIFT);
	}

	ox::video::S3DVertex* target = &InterpolateBuffer[0];
	ox::video::S3DVertex* first = &FrameList[firstFrame][0];
	ox::video::S3DVertex* second = &FrameList[secondFrame][0];

	int count = FrameList[firstFrame].size();

	// interpolate both frames
	for (int i=0; i<count; ++i)
	{
		target->Pos = (second->Pos - first->Pos) * div + first->Pos;
		target->Normal = (second->Normal - first->Normal) * div + first->Normal;

		++target;
		++first;
		++second;
	}

	//update bounding box
	BoundingBox = BoxList[secondFrame].getInterpolated(BoxList[firstFrame], div);
}



//! returns max element
inline int CAnimatedMeshMD2::max(int a, int b)
{
	return a>b ? a : b;
}


//! loads an md3 file
bool CAnimatedMeshMD2::loadFile(ox::io::IReadFile* file)
{
	if (!file)
		return false;

	SMD2Header header;
	char* frames = 0;

	file->read(&header, sizeof(SMD2Header));

	if (header.magic != MD2_MAGIC_NUMBER || header.version != MD2_VERSION)
	{
		os::Printer::log("MD2 Loader: Wrong file header", file->getFileName(), ox::event::ELL_WARNING);
		return false;
	}

	// create Memory for indices and frames

	Indices.reserve(header.numTriangles);
	FrameList = new ox::TArray<ox::video::S3DVertex>[header.numFrames];
	FrameCount = header.numFrames;

	int i;

	for (i=0; i<header.numFrames; ++i)
		FrameList[i].reserve(header.numVertices);

	// read TextureCoords

	file->seek(header.offsetTexcoords, false);
	SMD2TextureCoordinate* textureCoords = new SMD2TextureCoordinate[header.numTexcoords];

	if (!file->read(textureCoords, sizeof(SMD2TextureCoordinate)*header.numTexcoords))
	{
		os::Printer::log("MD2 Loader: Error reading TextureCoords.", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	// read Triangles

	file->seek(header.offsetTriangles, false);

	SMD2Triangle *triangles = new SMD2Triangle[header.numTriangles];
	if (!file->read(triangles, header.numTriangles *sizeof(SMD2Triangle)))
	{
		os::Printer::log("MD2 Loader: Error reading triangles.", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	// read Vertices

	char buffer[MD2_MAX_VERTS*4+128];
	SMD2Frame* frame = (SMD2Frame*)buffer;

	ox::TArray< ox::core::CVector3d<float> >* vertices = new ox::TArray< ox::core::CVector3d<float> >[header.numFrames];
	ox::TArray< ox::core::CVector3d<float> >* normals = new ox::TArray< ox::core::CVector3d<float> >[header.numFrames];

	file->seek(header.offsetFrames, false);

	for (i = 0; i<header.numFrames; ++i)
	{
		// read vertices

		vertices[i].reserve(header.numVertices);
		file->read(frame, header.frameSize);

		// store frame data

		SFrameData fdata;
		fdata.begin = i;
		fdata.end = i;
		fdata.fps = 7;

		if (frame->name[0])
		{
			for (int s = 0; frame->name[s]!=0 && (frame->name[s] < '0' ||
				frame->name[s] > '9'); ++s)
				fdata.name += frame->name[s];

			if (!FrameData.empty() && FrameData[FrameData.size()-1].name == fdata.name)
				++FrameData[FrameData.size()-1].end;
			else
				FrameData.push_back(fdata);
		}

		// add vertices

		for (int j=0; j<header.numVertices; ++j)
		{
			ox::core::CVector3d<float> v;
			v.X = (float)frame->vertices[j].vertex[0] * frame->scale[0] + frame->translate[0];
			v.Z = (float)frame->vertices[j].vertex[1] * frame->scale[1] + frame->translate[1];
			v.Y = (float)frame->vertices[j].vertex[2] * frame->scale[2] + frame->translate[2];

			vertices[i].push_back(v);

			int normalidx = frame->vertices[j].lightNormalIndex;
			if (normalidx > 0 && normalidx < Q2_VERTEX_NORMAL_TABLE_SIZE)
			{
				v.X = Q2_VERTEX_NORMAL_TABLE[normalidx][0];
				v.Y = Q2_VERTEX_NORMAL_TABLE[normalidx][1];
				v.Z = Q2_VERTEX_NORMAL_TABLE[normalidx][2];
			}

			normals[i].push_back(v);
		}

		// calculate bounding boxes
		if (header.numVertices)
		{
			ox::core::CAabbox3d<float> box;
			box.reset(vertices[i][0]);

			for (int j=1; j<header.numVertices; ++j)
				box.addInternalPoint(vertices[i][j]);

			BoxList.push_back(box);
		}

	}

	// put triangles into frame list

	float dmaxs = 1.0f/(header.skinWidth);
	float dmaxt = 1.0f/(header.skinHeight);

	ox::video::S3DVertex vtx;
	vtx.Color = ox::video::SColor(255,255,255,255);

	for (int f = 0; f<header.numFrames; ++f)
	{
		ox::TArray< ox::core::CVector3d<float> >& vert = vertices[f];

		for (int t=0; t<header.numTriangles; ++t)
		{
			for (int n=0; n<3; ++n)
			{
				vtx.Pos = vert[triangles[t].vertexIndices[n]];
				vtx.TCoords.X = (textureCoords[triangles[t].textureIndices[n]].s + 0.5f) * dmaxs;
				vtx.TCoords.Y = (textureCoords[triangles[t].textureIndices[n]].t + 0.5f) * dmaxt;
				vtx.Normal = normals[f][triangles[t].vertexIndices[n]];
				FrameList[f].push_back(vtx);
			}
		}
	}

	// create indices

	Indices.reserve(header.numVertices);
	int count = header.numTriangles*3;
	for (int n=0; n<count; n+=3)
	{
		Indices.push_back(n);
		Indices.push_back(n+1);
		Indices.push_back(n+2);
	}

	//calculateNormals();

	// reallocate interpolate buffer
	if (header.numFrames)
	{
#ifdef HARVEST_PORT
		InterpolateBuffer.resize(FrameList[0].size());
#else
		InterpolateBuffer.reserve(FrameList[0].size());
#endif

		int count = FrameList[0].size();
		for (i=0; i<count; ++i)
		{
			InterpolateBuffer[i].TCoords = FrameList[0][i].TCoords;
			InterpolateBuffer[i].Color = vtx.Color;
		}
	}

	// clean up

	delete [] normals;
	delete [] vertices;
	delete [] triangles;
	delete [] textureCoords;
	TriangleCount = Indices.size() / 3;

	// return

	calculateBoundingBox();

	return true;
}



//! calculates the bounding box
void CAnimatedMeshMD2::calculateBoundingBox()
{
	BoundingBox.reset(0,0,0);

	if (FrameCount)
	{
		unsigned int defaultFrame = 1;

		if (defaultFrame>=FrameCount)
			defaultFrame = 0;

			for (unsigned int j=0; j<FrameList[defaultFrame].size(); ++j)
				BoundingBox.addInternalPoint(FrameList[defaultFrame][j].Pos);
	}
}


//! sets a flag of all contained materials to a new value
void CAnimatedMeshMD2::setMaterialFlag(ox::video::E_MATERIAL_FLAG flag, bool newvalue)
{
	Material.Flags[flag] = newvalue;
}



//! returns an axis aligned bounding box
const ox::core::CAabbox3d<float>& CAnimatedMeshMD2::getBoundingBox() const
{
	return BoundingBox;
}



//! returns an axis aligned bounding box
ox::core::CAabbox3d<float>& CAnimatedMeshMD2::getBoundingBox()
{
	return BoundingBox;
}



//! calculates normals
void CAnimatedMeshMD2::calculateNormals()
{
	for (unsigned int i=0; i<FrameCount; ++i)
	{
		ox::video::S3DVertex* vtx = &FrameList[i][0];

		for (unsigned int j=0; j<Indices.size(); j+=3)
		{
			ox::core::CPlane3d<float> plane(
				vtx[Indices[j]].Pos, vtx[Indices[j+1]].Pos,	vtx[Indices[j+2]].Pos);

			vtx[Indices[j]].Normal = plane.Normal;
			vtx[Indices[j+1]].Normal = plane.Normal;
			vtx[Indices[j+2]].Normal = plane.Normal;
		}
	}
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CAnimatedMeshMD2::getMeshType() const
{
	return EAMT_MD2;
}


//! Returns frame loop data for a special MD2 animation type.
void CAnimatedMeshMD2::getFrameLoop(EMD2_ANIMATION_TYPE l,
									int& outBegin, int& outEnd, int& outFPS) const
{
	if (l < 0 || l >= EMAT_COUNT)
		return;

	outBegin = MD2AnimationTypeList[l].begin << MD2_FRAME_SHIFT;
	outEnd = MD2AnimationTypeList[l].end << MD2_FRAME_SHIFT;
	outFPS = MD2AnimationTypeList[l].fps * 5;
}


//! Returns frame loop data for a special MD2 animation type.
bool CAnimatedMeshMD2::getFrameLoop(const char* name,
	int& outBegin, int&outEnd, int& outFps) const
{
	for (int i=0; i<(int)FrameData.size(); ++i)
		if (FrameData[i].name == name)
		{
			outBegin = FrameData[i].begin << MD2_FRAME_SHIFT;
			outEnd = FrameData[i].end << MD2_FRAME_SHIFT;
			outFps = FrameData[i].fps * 5;
			return true;
		}

	return false;
}



//! Returns amount of md2 animations in this file.
int CAnimatedMeshMD2::getAnimationCount() const
{
	return FrameData.size();
}


//! Returns name of md2 animation.
const char* CAnimatedMeshMD2::getAnimationName(int nr) const
{
	if (nr < 0 || nr >= (int)FrameData.size())
		return 0;

	return FrameData[nr].name.c_str();
}


} // end namespace scene
} // end namespace daisy
