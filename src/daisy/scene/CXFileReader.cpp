// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXFileReader.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/algo/CArrayFunctions.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CString.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CXFileReader.h"
#include "daisy/os.h"
#include "daisy/other/fast_atof.h"
#include <stdlib.h>
#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

CXFileReader::CXFileReader(ox::io::IReadFile* file)
: ErrorHappened(false), Buffer(0), Size(0), P(0), End(0)
{
	if (!file)
	{
		ErrorHappened = true;
		return;
	}

	if (!readFileIntoMemory(file))
	{
		ErrorHappened = true;
		return;
	}

	if (!parseFile())
	{
		ErrorHappened = true;
		return;
	}

	#ifdef _XREADER_DEBUG
		validateMesh(&RootFrame);
	#endif

	computeGlobalFrameMatrizes(RootFrame, 0);
}


CXFileReader::~CXFileReader()
{
	if (Buffer)
		delete [] Buffer;

	for (unsigned int i=0; i<AnimationSets.size(); ++i)
		for (unsigned int j=0; j<AnimationSets[i].Animations.size(); ++j)
			for (unsigned int k=0; k<AnimationSets[i].Animations[j].Keys.size(); ++k)
				AnimationSets[i].Animations[j].Keys[k].del();
}


//! Returns an error occured during reading the file
bool CXFileReader::errorHappened()
{
	return ErrorHappened;
}


//! checks if there was an error loading the mesh
bool CXFileReader::validateMesh(SXFrame* frame)
{
	bool error = false;
	char tmp[1024];

	// validate all meshes in this frame

	for (unsigned int m=0; m<frame->Meshes.size(); ++m)
	{
		int vcnt = frame->Meshes[m].Vertices.size();
		int icnt = frame->Meshes[m].Indices.size();
		int ncnt = frame->Meshes[m].Normals.size();
		int nicnt = frame->Meshes[m].NormalIndices.size();

		int i;

		// validate indices in mesh

		for (i=0; i<icnt; ++i)
		{
			if (frame->Meshes[m].Indices[i] < 0)
			{
				sprintf(tmp, "XLoader error: index %d smaller than 0 in mesh %d: %d, frame '%s'",
					i, m, frame->Meshes[m].Indices[i], frame->Name.c_str());
				os::Printer::log(tmp, ox::event::ELL_ERROR);
				error = true;
			}

			if (frame->Meshes[m].Indices[i] > vcnt-1)
			{
				sprintf(tmp, "XLoader error: invalid index %d in mesh %d: %d, frame '%s'",
					i, m, frame->Meshes[m].Indices[i], frame->Name.c_str());
				os::Printer::log(tmp, ox::event::ELL_ERROR);
				error = true;
			}
		}

		// validate normal indices

		for (i=0; i<nicnt; ++i)
		{
			if (frame->Meshes[m].NormalIndices[i] < 0)
			{
				sprintf(tmp, "XLoader error: normal index %d smaller than 0 in mesh %d: %d, frame '%s'",
					i, m, frame->Meshes[m].NormalIndices[i], frame->Name.c_str());
				os::Printer::log(tmp, ox::event::ELL_ERROR);
				error = true;
			}

			if (frame->Meshes[m].NormalIndices[i] > ncnt-1)
			{
				sprintf(tmp, "XLoader error: invalid normal index %d in mesh %d: %d, frame '%s'",
					i, m, frame->Meshes[m].NormalIndices[i], frame->Name.c_str());
				os::Printer::log(tmp, ox::event::ELL_ERROR);
				error = true;
			}
		}
	}


	// validate child frames
	for (unsigned int i=0; i<frame->ChildFrames.size(); ++i)
		if (!validateMesh(&frame->ChildFrames[i]))
			error = true;

	return error;
}


//! Reads file into memory
bool CXFileReader::readFileIntoMemory(ox::io::IReadFile* file)
{
	int Size = file->getSize();
	if (Size < 12)
	{
		os::Printer::log("X File is too small.", ox::event::ELL_WARNING);
		return false;
	}

	Buffer = new char[Size];

	//! read all into memory
	if (file->read(Buffer, Size) != Size)
	{
		os::Printer::log("Could not read from x file.", ox::event::ELL_WARNING);
		return false;
	}

	End = Buffer + Size;

	//! check header xof 0302
	if (Buffer[0] != 'x' || Buffer[1] != 'o' ||
		Buffer[2] != 'f' || Buffer[3] != ' ')
	{
		os::Printer::log("Not an x file, wrong header.", ox::event::ELL_WARNING);
		return false;
	}

	//! read minor and major version
	char tmp[3];
	tmp[2] = 0x0;
	tmp[0] = Buffer[4];
	tmp[1] = Buffer[5];
	MajorVersion = strtol(tmp, &P, 10);

	tmp[0] = Buffer[6];
	tmp[1] = Buffer[7];
	MinorVersion = strtol(tmp, &P, 10);

	//! read format
	if (Buffer[8] != 't' || Buffer[9] != 'x' ||
		Buffer[10] != 't' || Buffer[11] != ' ')
	{
		os::Printer::log("Only text encoded x files supported currently.", ox::event::ELL_WARNING);
		return false;
	}

	//! read float size
	if (Buffer[12] == '0' || Buffer[13] != '0' ||
		Buffer[14] != '3' || Buffer[15] != '2')
		FloatSize = 32;
	else
		FloatSize = 64;

	P = &Buffer[16];

	readUntilEndOfLine();

	return true;
}

//! Parses the file
bool CXFileReader::parseFile()
{
	while(parseDataObject())
	{
		// loop
	}

    return true;
}


//! Parses the next Data object in the file
bool CXFileReader::parseDataObject()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectFrame(SXFrame& frame)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectTransformationMatrix(ox::core::CMatrix4 &mat)
{
	#ifdef _XREADER_DEBUG
    os::Printer::log("CXFileReader: Reading Transformation Matrix", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Transformation Matrix found in x file", ox::event::ELL_WARNING);
		return false;
	}

	for (int i=0; i<4; ++i)
		for (int j=0; j<4; ++j)
		{
			findNextNoneWhiteSpaceNumber();
			P = core::fast_atof_move(P, mat(j,i));
		}

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Transformation Matrix found in x file", ox::event::ELL_WARNING);
		return false;
	}

	findNextNoneWhiteSpace();
	if (P[0] != '}')
	{
		os::Printer::log("No closing brace in Transformation Matrix found in x file", ox::event::ELL_WARNING);
		return false;
	}
	++P;

	return true;
}


bool CXFileReader::parseDataObjectTemplate()
{
	#ifdef _XREADER_DEBUG
    os::Printer::log("CXFileReader: Reading template", ox::event::ELL_INFORMATION);
	#endif

	// parse a template data object. Currently not stored.
	ox::core::CString<char> name = getNextToken();

	// ignore left delimiter
	if (getNextToken() != "{")
	{
		os::Printer::log("Left delimiter in template data object missing.",
			name.c_str(), ox::event::ELL_ERROR);
		return false;
	}

	// read UUID
	ox::core::CString<char> uuid = getNextToken();

	// read and ignore data members
	while(true)
	{
		ox::core::CString<char> s = getNextToken();
		if (s == "}")
			break;

		if (s.size() == 0)
			return false;
	}

	return true;
}


bool CXFileReader::parseDataObjectMesh(SXMesh &mesh)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectSkinWeights(SXSkinWeight& weights)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectSkinMeshHeader(SXSkinMeshHeader& header)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectMeshMaterialList(SXMeshMaterialList& mlist,
												   int triangulatedIndexCount,
												   ox::TArray< int >& indexCountPerFace)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


bool CXFileReader::parseDataObjectMaterial(SXMaterial& material)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! reads a x file style string
bool CXFileReader::getNextTokenAsString(ox::core::CString<char>& out)
{
	ox::core::CString<char> s = getNextToken();
	int sl = s.size();

	if (sl < 3)
		return false;

	if (s[0] != '"' || s[sl-1] != ';' || s[sl-2] != '"')
		return false;

	out = s.subString(1, sl-3);
	return true;
}


bool CXFileReader::parseDataObjectAnimationSet(SXAnimationSet& set)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading animation set", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject(&set.AnimationName))
	{
		os::Printer::log("No opening brace in Animation Set found in x file", ox::event::ELL_WARNING);
		return false;
	}

	while(true)
	{
		ox::core::CString<char> objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Animation set in x file.", ox::event::ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // animation set finished
		}
		else
		if (objectName == "Animation")
		{
			set.Animations.push_back(SXAnimation());
			if (!parseDataObjectAnimation(set.Animations[set.Animations.size()-1]))
				return false;
		}
		else
		{
			os::Printer::log("Unknown data object in animation set in x file", objectName.c_str(), ox::event::ELL_INFORMATION);
			if (!parseUnknownDataObject())
				return false;
		}

	}

	return true;
}


bool CXFileReader::parseDataObjectAnimation(SXAnimation& anim)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading animation", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Animation found in x file", ox::event::ELL_WARNING);
		return false;
	}

	anim.closed = true;
	anim.linearPositionQuality = true;

	while(true)
	{
		ox::core::CString<char> objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Animation in x file.", ox::event::ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // animation finished
		}
		else
		if (objectName == "AnimationKey")
		{
			anim.Keys.push_back(SXAnimationKey());
			if (!parseDataObjectAnimationKey(anim.Keys[anim.Keys.size()-1]))
				return false;
		}
		else
		if (objectName == "AnimationOptions")
		{
			//TODO: parse options.
			if (!parseUnknownDataObject())
				return false;
		}
		else
		if (objectName == "{")
		{
			// read frame name
			anim.FrameName = getNextToken();
			ox::core::CString<char> end = getNextToken();
			if (end.size() == 0 || end != "}")
			{
				os::Printer::log("Unexpected ending found in Animation in x file.", ox::event::ELL_WARNING);
				return false;
			}
		}
		else
		{
			if (objectName.size()>2 && objectName[0] == '{' &&
				objectName[objectName.size()-1] == '}')
			{
				anim.FrameName = objectName.subString(1,objectName.size()-2);
			}
			else
			{
				os::Printer::log("Unknown data object in animation in x file", objectName.c_str(), ox::event::ELL_INFORMATION);
				if (!parseUnknownDataObject())
					return false;
			}
		}

	}

	return true;
}


bool CXFileReader::parseDataObjectAnimationKey(SXAnimationKey& animkey)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading animation key", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Animation Key found in x file", ox::event::ELL_WARNING);
		return false;
	}

	// read key type
	findNextNoneWhiteSpaceNumber();
	animkey.keyType = strtol(P, &P, 10);

	if (!(animkey.keyType == 0 || animkey.keyType == 1 ||
		animkey.keyType == 2 || animkey.keyType == 3 ||
		animkey.keyType == 4))
	{
		os::Printer::log("Unknown key type found in Animation Key in x file", ox::event::ELL_WARNING);
		return false;
	}

	// read number of keys
	findNextNoneWhiteSpaceNumber();
	animkey.numberOfKeys = strtol(P, &P, 10);

	animkey.init();

	// read keys
	switch(animkey.keyType)
	{
	case 0:
		{
			//read quaternions
			for (int i=0; i<animkey.numberOfKeys; ++i)
			{
				// read time
				findNextNoneWhiteSpaceNumber();
				animkey.time[i] = strtol(P, &P, 10);

				// read count
				++P;
				if (strtol(P, &P, 10) != 4)
				{
					os::Printer::log("Expected 4 numbers in animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				++P;
				animkey.getQuaternion(i).W = -readFloat();
				++P;
				animkey.getQuaternion(i).X = -readFloat();
				++P;
				animkey.getQuaternion(i).Y = -readFloat();
				++P;
				animkey.getQuaternion(i).Z = -readFloat();


				if (P[0] != ';' || P[1] != ';')
				{
					os::Printer::log("No following two semicolons missing after quaternion animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				P+=3;
			}
		}
		break;
	case 1:
	case 2:
		{
			// read vectors
			for (int i=0; i<animkey.numberOfKeys; ++i)
			{
				// read time
				findNextNoneWhiteSpaceNumber();
				animkey.time[i] = strtol(P, &P, 10);

				// read count
				++P;
				if (strtol(P, &P, 10) != 3)
				{
					os::Printer::log("Expected 3 numbers in animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				++P;
				animkey.getVector(i).X = readFloat();
				++P;
				animkey.getVector(i).Y = readFloat();
				++P;
				animkey.getVector(i).Z = readFloat();

				if (P[0] != ';' || P[1] != ';')
				{
					os::Printer::log("No following two semicolons missing after vector animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				P+=3;
			}
		}
		break;
	case 3:
	case 4:
		{
			// read matrix

			for (int i=0; i<animkey.numberOfKeys; ++i)
			{
				// read time
				findNextNoneWhiteSpaceNumber();
				animkey.time[i] = strtol(P, &P, 10);

				// read count
				++P;
				if (strtol(P, &P, 10) != 16)
				{
					os::Printer::log("Expected 16 numbers in animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				// read matrix
				for (int n=0; n<4; ++n)
					for (int m=0; m<4; ++m)
					{
						findNextNoneWhiteSpaceNumber();
						animkey.getMatrix(i)(m,n) = readFloat();
					}

				if (P[0] != ';' || P[1] != ';')
				{
					os::Printer::log("No following two semicolons missing after matrix animation key in x file", ox::event::ELL_WARNING);
					return false;
				}

				P+=3;
			}
		}
		break;
	} // end switch


	ox::core::CString<char> objectName = getNextToken();

	if (objectName != "}")
	{
		os::Printer::log("No closing brace in animation key in x file", objectName.c_str(), ox::event::ELL_INFORMATION);
		return false;
	}

	return true;
}


bool CXFileReader::parseDataObjectTextureFilename(ox::core::CString<char>& texturename)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading texture filename", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Texture filename found in x file", ox::event::ELL_WARNING);
		return false;
	}

	if (!getNextTokenAsString(texturename))
	{
		os::Printer::log("Unknown syntax while reading texture filename string in x file", ox::event::ELL_WARNING);
		return false;
	}

	findNextNoneWhiteSpace();
	if (P[0] != '}')
	{
		os::Printer::log("No closing brace in Texture filename found in x file", ox::event::ELL_WARNING);
		return false;
	}
	++P;

	return true;
}


bool CXFileReader::parseDataObjectMeshNormals(ox::TArray<ox::core::CVector3d<float> >& normals,
											  ox::TArray< int >& normalIndices,
											  int triangulatedIndexCount,
											  ox::TArray< int >& indexCountPerFace)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading mesh normals", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Normals found in x file", ox::event::ELL_WARNING);
		return false;
	}

	// read count

	findNextNoneWhiteSpaceNumber();
	int nNormals = strtol(P, &P, 10);
	normals.resize(nNormals);

	// read normals

	for (int i=0; i<nNormals; ++i)
	{
		findNextNoneWhiteSpaceNumber();
		normals[i].X = readFloat();
		++P;
		normals[i].Y = readFloat();
		++P;
		normals[i].Z = readFloat();
	}

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Normals Array found in x file", ox::event::ELL_WARNING);
		return false;
	}

	// read face normal indices

	findNextNoneWhiteSpaceNumber();
	int nFNormals = strtol(P, &P, 10);
	normalIndices.resize(triangulatedIndexCount);
	int normalidx = 0;
	ox::TArray<int> polygonfaces;

	for (int k=0; k<nFNormals; ++k)
	{
		findNextNoneWhiteSpaceNumber();
		int fcnt = strtol(P, &P, 10);
		int triangles = fcnt - 2;
		int indexcount = triangles * 3;

		if (indexcount != indexCountPerFace[k])
		{
			os::Printer::log("Not matching normal and face index count found in x file", ox::event::ELL_WARNING);
			return false;
		}

		if (indexcount == 3)
		{
			// default, only one triangle in this face
			for (int h=0; h<3; ++h)
			{
				++P;
				normalIndices[normalidx++] = strtol(P, &P, 10);
			}
		}
		else
		{
			// multiple triangles in this face
			polygonfaces.resize(fcnt);

			for (int h=0; h<fcnt; ++h)
			{
				++P;
				polygonfaces[h] = strtol(P, &P, 10);
			}

			for (int jk=0; jk<triangles; ++jk)
			{
				normalIndices[normalidx++] = polygonfaces[0];
				normalIndices[normalidx++] = polygonfaces[jk+1];
				normalIndices[normalidx++] = polygonfaces[jk+2];
			}
		}
	}

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Face Normals Array found in x file", ox::event::ELL_WARNING);
		return false;
	}

	findNextNoneWhiteSpace();
	if (P[0] != '}')
	{
		os::Printer::log("No closing brace in Mesh Normals found in x file", ox::event::ELL_WARNING);
		return false;
	}
	++P;

	return true;
}


bool CXFileReader::parseDataObjectMeshTextureCoords(
	ox::TArray<ox::core::CVector2d<float> >& textureCoords)
{
	#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading mesh texture coordinates", ox::event::ELL_INFORMATION);
	#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Texture Coordinates found in x file", ox::event::ELL_WARNING);
		return false;
	}

	findNextNoneWhiteSpaceNumber();
	int nCoords = strtol(P, &P, 10);
	textureCoords.resize(nCoords);

	for (int i=0; i<nCoords; ++i)
	{
		findNextNoneWhiteSpaceNumber();
		textureCoords[i].X = readFloat();
		++P;
		textureCoords[i].Y = readFloat();
	}

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Texture Coordinates Array found in x file", ox::event::ELL_WARNING);
		return false;
	}

	findNextNoneWhiteSpace();
	if (P[0] != '}')
	{
		os::Printer::log("No closing brace in Mesh Texture Coordinates Array found in x file", ox::event::ELL_WARNING);
		return false;
	}
	++P;

	return true;
}



bool CXFileReader::parseUnknownDataObject()
{
	// find opening delimiter

	while(true)
	{
		ox::core::CString<char> t = getNextToken();

		if (t.size() == 0)
			return false;

		if (t == "{")
			break;
	}

	int counter = 1;

	// parse until closing delimiter

	while(counter)
	{
		ox::core::CString<char> t = getNextToken();

		if (t.size() == 0)
			return false;

		if (t == "{")
			++counter;
		else
		if (t == "}")
			--counter;
	}

	return true;
}



//! checks for two following semicolons, returns false if they are not there
bool CXFileReader::checkForTwoFollowingSemicolons()
{
	for (int k=0; k<2; ++k)
	{
		findNextNoneWhiteSpace();
		if (P[0] != ';')
			return false;
		++P;
	}

	return true;
}


//! reads header of dataobject including the opening brace.
//! returns false if error happened, and writes name of object
//! if there is one
bool CXFileReader::readHeadOfDataObject(ox::core::CString<char>* outname)
{
	ox::core::CString<char> nameOrBrace = getNextToken();
	if (nameOrBrace != "{")
	{
		if (outname)
			(*outname) = nameOrBrace;

		if (nameOrBrace.size() != 0 &&
			nameOrBrace[nameOrBrace.size()-1] == '{')
		{
			(*outname) = nameOrBrace.subString(0, nameOrBrace.size()-1);
			return true;
		}

		nameOrBrace = getNextToken();
		if (nameOrBrace != "{")
			return false;
	}

	return true;
}


//! returns next parseable token. Returns empty string if no token there
ox::core::CString<char> CXFileReader::getNextToken()
{
	ox::core::CString<char> s;
	findNextNoneWhiteSpace();

	if (P >= End)
		return s;

	while(P < End && P[0]!=' ' && P[0]!='\n' && P[0]!='\r' && P[0]!='\t')
	{
		s.append(P[0]);
		++P;
	}

	return s;
}


//! places pointer to next begin of a token, which must be a number,
// and ignores comments
void CXFileReader::findNextNoneWhiteSpaceNumber()
{
	while(1)
	{
		while(P < End && P[0]!='-' &&
				((P[0]==' ' || P[0]=='\n' || P[0]=='\r' || P[0]=='\t') ||
				  (P[0] < '0' || P[0] > '9')))
			++P;

		if (P >= End)
			return;

		// check if this is a comment
		if ((P[0] == '/' && P[1] == '/') ||
			P[0] == '#')
			readUntilEndOfLine();
		else
			break;
	}

}

// places pointer to next begin of a token, and ignores comments
void CXFileReader::findNextNoneWhiteSpace()
{
	while(1)
	{
		while(P < End && (P[0]==' ' || P[0]=='\n' || P[0]=='\r' || P[0]=='\t'))
			++P;

		if (P >= End)
			return;

		// check if this is a comment
		if ((P[0] == '/' && P[1] == '/') ||
			P[0] == '#')
			readUntilEndOfLine();
		else
			break;
	}
}


void CXFileReader::readUntilEndOfLine()
{
	while(P < End)
	{
		if (P[0] == '\n')
		{
			++P;
			return;
		}

		++P;
	}
}


//! Returns if the loaded mesh is static
bool CXFileReader::isStaticMesh()
{
	return AnimationSets.empty();
}

//! returns count of animations
int CXFileReader::getAnimationSetCount()
{
	return AnimationSets.size();
}

//! returns a specific animation set
CXFileReader::SXAnimationSet& CXFileReader::getAnimationSet(int i)
{
	return AnimationSets[i];
}

//! returns the root frame of the mesh
CXFileReader::SXFrame& CXFileReader::getRootFrame()
{
	return RootFrame;
}


void CXFileReader::computeGlobalFrameMatrizes(SXFrame& frame, SXFrame* parent)
{
    if (!parent)
		frame.GlobalMatrix = frame.LocalMatrix;
	else
		frame.GlobalMatrix = parent->GlobalMatrix * frame.LocalMatrix;

#ifdef _XREADER_DEBUG
	if (frame.Name != "")
	{
		char tmp[255];
		sprintf(tmp, "CXFileReader: Frame %s ", frame.Name);
		os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		for (int i=0; i<4; ++i)
		{
			sprintf(tmp, "    %f, %f, %f, %f",
				frame.LocalMatrix(i,0),
				frame.LocalMatrix(i,1),
				frame.LocalMatrix(i,2),
				frame.LocalMatrix(i,3));
			os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		}
	}
#endif

	/*if (frame.Name == "")
	{
		// frames without name or with mesh data in them (not sure exactly)
		// seem to have to be ignored. TODO: check this out

		for (unsigned int c=0; c<frame.ChildFrames.size(); ++c)
			computeGlobalFrameMatrizes(frame.ChildFrames[c], parent);
	}
	else*/
	for (unsigned int c=0; c<frame.ChildFrames.size(); ++c)
		computeGlobalFrameMatrizes(frame.ChildFrames[c], &frame);
}

inline int CXFileReader::readInt()
{
	float ftmp;
	P = core::fast_atof_move(P, ftmp);
	return (int)ftmp;
}

inline float CXFileReader::readFloat()
{
	float ftmp;
	P = core::fast_atof_move(P, ftmp);
	return ftmp;
}

} // end namespace scene
} // end namespace daisy
