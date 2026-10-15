// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXFileReader.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_X_FILE_READER_H_INCLUDED__
#define __C_X_FILE_READER_H_INCLUDED__

#include "ox/io/IReadFile.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SLight.h"
#include "ox/IUnknown.h"
#include "ox/core/CString.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"
#include "ox/TArray.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CQuaternion.h"

//#define _XREADER_DEBUG

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! This is a x file reader. It only reads data from ascii x files.
class CXFileReader : public ox::IUnknown
{
public:

	struct SXAnimationSet;
	struct SXFrame;

	CXFileReader(ox::io::IReadFile* file);
	~CXFileReader();

	//! Returns an error occured during reading the file
	bool errorHappened();

	//! Returns if the loaded mesh is static
	bool isStaticMesh();

	//! returns count of animations
	int getAnimationSetCount();

	//! returns a specific animation set
	SXAnimationSet& getAnimationSet(int i);

	//! returns the root frame of the mesh
	SXFrame& getRootFrame();



	struct SXMaterial
	{
		ox::video::SColorf FaceColor; // note: RGBA
		float Power;
		ox::video::SColorf Specular; // RGB
		ox::video::SColorf Emissive; // RGB
		ox::core::CString<char> TextureFileName;
	};

	struct SXMeshMaterialList
	{
		ox::TArray<int> FaceIndices; // index of material for each face
		ox::TArray<SXMaterial> Materials; // material array
	};

	struct SXIndexedColor
	{
		int Index;
		ox::video::SColor Color;
	};

	struct SXSkinMeshHeader
	{
		int MaxSkinWeightsPerVertex; // Maximum number of transforms that affect a vertex in the mesh.
		int MaxSkinWeightsPerFace; // Maximum number of unique transforms that affect the three vertices of any face.
		int BoneCount; // Number of bones that affect vertices in this mesh.
	};

	struct SXWeight
	{
		SXWeight() {};
		SXWeight(int vtidx): VertexIndex(vtidx) {}

		int VertexIndex;
		float Weight;

		bool operator <(const SXWeight& other) const
		{
			return VertexIndex < other.VertexIndex;
		}
	};

	struct SXSkinWeight
	{
		ox::core::CString<char> TransformNodeName; // name of the bone
		ox::TArray< SXWeight > Weights;
		ox::core::CMatrix4 MatrixOffset; // transforms the mesh vertices to the space of the bone
									// When concatenated to the bone's transform, this provides the
									// world space coordinates of the mesh as affected by the bone
	};

	struct SXMesh
	{
		// this mesh contains triangulated texture data.
		// because in an .x file, faces can be made of more than 3
		// vertices, the indices data structure is triangulated during the
		// loading process. The IndexCountPerFace array is filled during
		// this triangulation process and stores how much indices belong to
		// every face. This data structure can be ignored, because all data
		// in this structure is triangulated.

		ox::core::CString<char> Name;
		ox::TArray<ox::core::CVector3d<float> > Vertices;
		ox::TArray< int > Indices; // triangle indices

		// optional:

		ox::TArray<ox::core::CVector2d<float> > TextureCoords;
		ox::TArray<ox::core::CVector3d<float> > Normals; // normals
		ox::TArray<int> NormalIndices; // amount is equal to Indices amount

		ox::TArray< int > IndexCountPerFace; // default 3, but could be more
		ox::TArray< SXIndexedColor > VertexColors;
		ox::TArray< SXSkinWeight > SkinWeights;
		SXMeshMaterialList MaterialList;
		SXSkinMeshHeader SkinMeshHeader;
	};

	struct SXFrame
	{
		ox::core::CString<char> Name;
		ox::core::CMatrix4 LocalMatrix;
		ox::core::CMatrix4 GlobalMatrix;

		ox::TArray<SXMesh> Meshes;
		ox::TArray<SXFrame> ChildFrames;
	};

	struct SXAnimationKey
	{
		SXAnimationKey()
			: time(0), data(0), keyType(-1)
		{
		}

		void del()
		{
			delete [] time;

			switch(keyType)
			{
			case -1: break;
			case 0 : delete [] (ox::core::CQuaternion*)data; break;
			case 1 :
			case 2 : delete [] (ox::core::CVector3d<float>*)data; break;
			case 3 :
			case 4 : delete [] (ox::core::CMatrix4*)data; break;
			}
		}

		void init()
		{
			time = new int[numberOfKeys];

			switch(keyType)
			{
			case 0 : data = new ox::core::CQuaternion[numberOfKeys]; break;
			case 1 :
			case 2 : data = new ox::core::CVector3d<float>[numberOfKeys]; break;
			case 3 :
			case 4 : data = new ox::core::CMatrix4[numberOfKeys]; break;
			}

		}

		inline ox::core::CMatrix4& getMatrix(int nr)
		{
			return ((ox::core::CMatrix4*)data)[nr];
		}

		inline ox::core::CVector3d<float>& getVector(int nr)
		{
			return ((ox::core::CVector3d<float>*)data)[nr];
		}

		inline ox::core::CQuaternion& getQuaternion(int nr)
		{
			return ((ox::core::CQuaternion*)data)[nr];
		}

		int keyType; // 0=rotation, 1=scale, 2=position, 3=matrix
		int numberOfKeys;

		int* time;
		void* data;
	};

	struct SXAnimation
	{
		ox::core::CString<char> FrameName;
		ox::TArray<SXAnimationKey> Keys;

		// optional, from AnimationOptions:
		bool closed; // default true
		bool linearPositionQuality; // if false: spline position quality
	};

	struct SXAnimationSet
	{
		ox::core::CString<char> AnimationName;
		ox::TArray<SXAnimation> Animations;
	};

private:

	//! Parses the file
	bool parseFile();

	//! Reads file into memory
	bool readFileIntoMemory(ox::io::IReadFile* file);

	//! Parses the next Data object in the file
	bool parseDataObject();

	//! places pointer to next begin of a token, and ignores comments
	void findNextNoneWhiteSpace();

	//! places pointer to next begin of a token, which must be a number,
	// and ignores comments
	void findNextNoneWhiteSpaceNumber();

	//! returns next parseable token. Returns empty string if no token there
	ox::core::CString<char> getNextToken();

	//! reads header of dataobject including the opening brace.
	//! returns false if error happened, and writes name of object
	//! if there is one
	bool readHeadOfDataObject(ox::core::CString<char>* outname=0);

	//! checks for two following semicolons, returns false if they are not there
	bool checkForTwoFollowingSemicolons();

	//! reads a x file style string
	bool getNextTokenAsString(ox::core::CString<char>& out);

	int readInt();
	float readFloat();

	bool parseDataObjectTemplate();
	bool parseDataObjectFrame(SXFrame &frame);
	bool parseDataObjectTransformationMatrix(ox::core::CMatrix4 &mat);
	bool parseDataObjectMesh(SXMesh &mesh);
	bool parseDataObjectMeshNormals(ox::TArray<ox::core::CVector3d<float> >& normals,
		ox::TArray< int >& normalIndices, int triangulatedIndexCount,
		ox::TArray< int >& indexCountPerFace);
	bool parseDataObjectMeshTextureCoords(ox::TArray<ox::core::CVector2d<float> >& textureCoords);
	bool parseDataObjectMeshMaterialList(SXMeshMaterialList& materiallist,
		int triangulatedIndexCount, ox::TArray< int >& indexCountPerFace);
	bool parseDataObjectMaterial(SXMaterial& material);
	bool parseDataObjectTextureFilename(ox::core::CString<char>& texturename);
	bool parseDataObjectSkinMeshHeader(SXSkinMeshHeader& header);
	bool parseDataObjectSkinWeights(SXSkinWeight& weights);
	bool parseDataObjectAnimationSet(SXAnimationSet& set);
	bool parseDataObjectAnimation(SXAnimation& anim);
	bool parseDataObjectAnimationKey(SXAnimationKey& animkey);
	bool parseUnknownDataObject();

	bool ErrorHappened;

	void readUntilEndOfLine();

	void computeGlobalFrameMatrizes(SXFrame& frame, SXFrame* parent);

	bool validateMesh(SXFrame* frame);

	int MajorVersion;
	int MinorVersion;

	char* Buffer;
	int Size;
	char FloatSize;
	char* P;
	char* End;

	SXFrame RootFrame;
	ox::TArray<SXAnimationSet> AnimationSets;
};

} // end namespace scene
} // end namespace daisy

#endif
