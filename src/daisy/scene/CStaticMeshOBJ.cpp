// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CStaticMeshOBJ.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColor.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CStaticMeshOBJ.h"
#include <stdlib.h>
#include <string.h>
#include "ox/core/CString.h"
#include "ox/scene/SMeshBuffer.h"
#include "daisy/os.h"
#include "daisy/other/fast_atof.h"


namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CStaticMeshOBJ::CStaticMeshOBJ()
{
}



//! destructor
CStaticMeshOBJ::~CStaticMeshOBJ()
{
}



//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
int CStaticMeshOBJ::getFrameCount()
{
	return 1;
}



//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CStaticMeshOBJ::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	return &Mesh;
}



//! loads an obj file
bool CStaticMeshOBJ::loadFile(ox::io::IReadFile* file)
{
	const int WORD_BUFFER_LENGHT = 255;
	char wordBuffer[WORD_BUFFER_LENGHT];

	SMeshBuffer *meshbuffer = new SMeshBuffer;

	ox::TArray< ox::core::CVector3d<float> > vertexBuffer;
	ox::TArray< ox::core::CVector2d<float> > textureCoordBuffer;
	ox::TArray< ox::core::CVector3d<float> > normalsBuffer;

	int filesize = (int)file->getSize();

	if (!filesize)
		return false;

	char* buf = new char[filesize];
	file->read((void*)buf, filesize);

	char* line = buf;

	while(line)
	{
		char* word = line;
		copyWord(wordBuffer, WORD_BUFFER_LENGHT, word);

		if (wordBuffer[0] == 'v' && wordBuffer[1] == 0x0)
		{
			// vector
			char* p1 = getNextWord(word);
			char* p2 = getNextWord(p1);
			char* p3 = getNextWord(p2);

			vertexBuffer.push_back(ox::core::CVector3d<float>(	core::fast_atof(p1),
														core::fast_atof(p2),
														core::fast_atof(p3)));
		}
		else
		if (wordBuffer[0] == 'v' && wordBuffer[1] == 't')
		{
			// texture coordinate
			char* p1 = getNextWord(word);
			char* p2 = getNextWord(p1);

			textureCoordBuffer.push_back(ox::core::CVector2d<float>(	core::fast_atof(p1),
																core::fast_atof(p2)));
		}
		else
		if (wordBuffer[0] == 'v' && wordBuffer[1] == 'n')
		{
			// vertex normal
			char* p1 = getNextWord(word);
			char* p2 = getNextWord(p1);
			char* p3 = getNextWord(p2);

			normalsBuffer.push_back(ox::core::CVector3d<float>(	core::fast_atof(p1),
															core::fast_atof(p2),
															core::fast_atof(p3)));
		}
		else
		if (wordBuffer[0] == 'f' && wordBuffer[1] == 0x0)
		{
			// face
			char faceBuf[20];
			copyWordLineEnd(wordBuffer, WORD_BUFFER_LENGHT, word);
			//copyWord(wordBuffer, WORD_BUFFER_LENGHT, word);

			const int MAX_FACE_POINT_COUNT = 40;
			unsigned int facePointCount = 0;
			int facePoints[MAX_FACE_POINT_COUNT][3];

			for (int k=0; k<MAX_FACE_POINT_COUNT; ++k)
			{
				facePoints[k][0] = -1;
				facePoints[k][1] = -1;
				facePoints[k][2] = -1;
			}

			char* face = getNextWord(wordBuffer);

			while(face && face[0])
			{
				copyWordSpaceEnd(faceBuf, WORD_BUFFER_LENGHT, face);

				//im facebuf steht jetzt die face.
				int idx = 0;
				int len = (int)strlen(faceBuf);
				int z = 0;

				while (idx < len)
				{
					facePoints[facePointCount][z] = atoi(&faceBuf[idx]);
					for (++idx; idx<len && faceBuf[idx] && faceBuf[idx-1]!='/'; ++idx);
					z++;
				}

				++facePointCount;

				if (facePointCount>=MAX_FACE_POINT_COUNT)
				{
					meshbuffer->drop();
					os::Printer::log("Face with more than 40 face points found s32 file. Not loading.", file->getFileName(), ox::event::ELL_ERROR);
					delete [] buf;
					return false;
				}

				face = getNextWord(face);
			}

			// now we've got a face.
			// and have to create some vertices and indices from them

			int currentVertexCount = meshbuffer->Vertices.size();

			for (unsigned int i=0; i<facePointCount; ++i)
			{
				ox::video::S3DVertex v;
				v.Color = ox::video::SColor(255,255,255,255);

				if (facePoints[i][2]-1 >= 0 && facePoints[i][2]-1 < (int)normalsBuffer.size())
				{
					v.Normal.X = normalsBuffer[ facePoints[i][2]-1 ].X;
					v.Normal.Y = normalsBuffer[ facePoints[i][2]-1 ].Y;
					v.Normal.Z = normalsBuffer[ facePoints[i][2]-1].Z;
				}
				else
				{
					v.Normal.X = v.Normal.Y = v.Normal.Z = 0;
				}

				if (facePoints[i][0]-1 >= 0 && facePoints[i][0]-1 < (int)vertexBuffer.size())
				{
					v.Pos.X  = vertexBuffer [ facePoints[i][0]-1 ].X;
					v.Pos.Y  = vertexBuffer [ facePoints[i][0]-1 ].Y;
					v.Pos.Z  = vertexBuffer [ facePoints[i][0]-1 ].Z;
				}
				else
				{
					v.Pos.X = v.Pos.Y = v.Pos.Z = 0;
				}

				if (facePoints[i][1]-1 >= 0 && facePoints[i][1]-1 < (int)textureCoordBuffer.size())
				{
					v.TCoords.X = textureCoordBuffer[ facePoints[i][1]-1].X;
					v.TCoords.Y = -textureCoordBuffer[ facePoints[i][1]-1].Y;
				}
				else
				{
					v.TCoords.X = v.TCoords.Y = 0;
				}


				meshbuffer->Vertices.push_back(v);
			}

			meshbuffer->Indices.push_back(0 + currentVertexCount);
			meshbuffer->Indices.push_back(1 + currentVertexCount);
			meshbuffer->Indices.push_back((facePointCount-1) + currentVertexCount);


			for (unsigned int jk=0; jk<facePointCount-3; ++jk)
			{
				meshbuffer->Indices.push_back(1 + currentVertexCount);
				meshbuffer->Indices.push_back((facePointCount-2-jk) + currentVertexCount);
				meshbuffer->Indices.push_back((facePointCount-1-jk) + currentVertexCount);
			}

		}
		else
		if (wordBuffer[0] == '#' || wordBuffer[0] == 'u' ||wordBuffer[0] =='g')
		{
			// comment
			// find end of line
			int i=0;
			while(line[i])
			{
				if (line[i]=='\n' || line[i]=='\r')
					break;

				++i;
			}

			line = &line[i];
		}


		line = getNextWord(line);
	}

	delete [] buf;

	meshbuffer->recalculateBoundingBox();
	Mesh.addMeshBuffer(meshbuffer);
	Mesh.recalculateBoundingBox();
	meshbuffer->drop();
	return true;
}


char* CStaticMeshOBJ::getFirstWord(char* buf)
{
	int i = 0;
	while(buf[i] && (buf[i]==' ' || buf[i]=='\n' || buf[i]=='\r' || buf[i]=='\t'))
		++i;

	return &buf[i];
}



void CStaticMeshOBJ::copyWord(char* outBuf, int bufLength, char* word)
{
	if (!word)
		return;

	int i = 0;
	while(word[i])
	{
		if (word[i]==' ' || word[i]=='\n' || word[i]=='\r' || word[i]=='\t')
			break;
		++i;
	}

	for (int j=0; j<i; ++j)
		outBuf[j] = word[j];

	outBuf[i] = 0x0;
}


void CStaticMeshOBJ::copyWordLineEnd(char* outBuf, int bufLength, char* word)
{
	if (!word)
		return;

	int i = 0;
	while(word[i])
	{
		if (word[i]=='\n')
			break;
		++i;
	}

	for (int j=0; j<i; ++j)
		outBuf[j] = word[j];

	outBuf[i] = 0x0;
}

void CStaticMeshOBJ::copyWordSpaceEnd(char* outBuf, int bufLength, char* word)
{
	if (!word)
		return;

	int i = 0;
	while(word[i])
	{
		if (word[i]==' ')
			break;
		++i;
	}

	for (int j=0; j<i; ++j)
		outBuf[j] = word[j];

	outBuf[i] = 0x0;
}



char* CStaticMeshOBJ::getNextWord(char* word)
{
	if (!word)
		return 0;

	int i = 0;
	while(word[i])
	{
		if (word[i]==' ' || word[i]=='\n' || word[i]=='\r' || word[i]=='\t')
			break;

		++i;
	}

	char* nextWord = getFirstWord(&word[i]);
	return nextWord == word ? 0 : nextWord;
}


//! Returns an axis aligned bounding box of the mesh.
//! \return ox::core::A bounding box of this mesh is returned.
const ox::core::CAabbox3d<float>& CStaticMeshOBJ::getBoundingBox() const
{
	return Mesh.getBoundingBox();
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CStaticMeshOBJ::getMeshType() const
{
	return EAMT_OBJ;
}


} // end namespace scene
} // end namespace daisy
