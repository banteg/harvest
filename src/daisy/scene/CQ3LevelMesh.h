// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CQ3LevelMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_Q3_LEVEL_MESH_H_INCLUDED__
#define __C_Q3_LEVEL_MESH_H_INCLUDED__

#include "ox/scene/IQ3LevelMesh.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/SMeshBufferLightMap.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IFileSystem.h"
#include "ox/scene/SMesh.h"
#include "ox/video/IVideoDriver.h"
#include "ox/core/CString.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CQ3LevelMesh : public IQ3LevelMesh
	{
	public:

		//! constructor
		CQ3LevelMesh(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver);

		//! destructor
		virtual ~CQ3LevelMesh();

		//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
		bool loadFile(ox::io::IReadFile* file);

		//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
		virtual int getFrameCount();

		//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
		virtual IMesh* getMesh(int frameInMs, int detailLevel=255, int startFrameLoop=-1, int endFrameLoop=-1);

		//! Returns an axis aligned bounding box of the mesh.
		//! \return ox::core::A bounding box of this mesh is returned.
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

	private:

		//! constructs a mesh from the quake 3 level file.
		void constructMesh();

		//! loads the textures
		void loadTextures();

		enum eLumps
		{
			kEntities = 0,     // Stores player/object positions, etc...
			kTextures,         // Stores texture information
			kPlanes,           // Stores the splitting planes
			kNodes,            // Stores the BSP nodes
			kLeafs,            // Stores the leafs of the nodes
			kLeafFaces,        // Stores the leaf's indices into the faces
			kLeafBrushes,      // Stores the leaf's indices into the brushes
			kModels,           // Stores the info of world models
			kBrushes,          // Stores the brushes info (for collision)
			kBrushSides,       // Stores the brush surfaces info
			kVertices,         // Stores the level vertices
			kMeshVerts,        // Stores the model vertices offsets
			kShaders,          // Stores the shader files (blending, anims..)
			kFaces,            // Stores the faces for the level
			kLightmaps,        // Stores the lightmaps for the level
			kLightVolumes,     // Stores extra world lighting information
			kVisData,          // Stores PVS and cluster info (visibility)
			kMaxLumps          // ox::core::A constant to store the number of lumps
		};

		struct tBSPLump
		{
			int offset;
			int length;
		}; 

		struct tBSPHeader
		{
			int strID;     // This should always be 'IBSP'
			int version;       // This should be 0x2e for Quake 3 files
		}; 

		struct tBSPVertex
		{
			float vPosition[3];      // (x, y, z) position. 
			float vTextureCoord[2];  // (u, v) texture coordinate
			float vLightmapCoord[2]; // (u, v) lightmap coordinate
			float vNormal[3];        // (x, y, z) normal vector
			unsigned char color[4];           // RGBA color for the vertex 
		};

		struct tBSPFace
		{
			int textureID;        // The index into the texture array 
			int effect;           // The index for the effects (or -1 = n/a) 
			int type;             // 1=polygon, 2=patch, 3=mesh, 4=billboard 
			int vertexIndex;      // The index into this face's first vertex 
			int numOfVerts;       // The number of vertices for this face 
			int meshVertIndex;    // The index into the first meshvertex 
			int numMeshVerts;     // The number of mesh vertices 
			int lightmapID;       // The texture index for the lightmap 
			int lMapCorner[2];    // The face's lightmap corner in the image 
			int lMapSize[2];      // The size of the lightmap section 
			float lMapPos[3];     // The 3D origin of lightmap. 
			float lMapBitsets[2][3]; // The 3D space for s and t unit vectors. 
			float vNormal[3];     // The face normal. 
			int size[2];          // The bezier patch dimensions. 
		};

		struct tBSPTexture
		{
			char strName[64];   // The name of the texture w/o the extension 
			unsigned int flags;          // The surface flags (unknown) 
			unsigned int contents;       // The content flags (unknown)
		};

		struct tBSPLightmap
		{
			unsigned char imageBits[128][128][3];   // The RGB data in a 128x128 image
		};

		struct tBSPNode
		{
			int plane;      // The index into the planes array 
			int front;      // The child index for the front node 
			int back;       // The child index for the back node 
			int mins[3];    // The bounding box min position. 
			int maxs[3];    // The bounding box max position. 
		}; 

		struct tBSPLeaf
		{
			int cluster;           // The visibility cluster 
			int area;              // The area portal 
			int mins[3];           // The bounding box min position 
			int maxs[3];           // The bounding box max position 
			int leafface;          // The first index into the face array 
			int numOfLeafFaces;    // The number of faces for this leaf 
			int leafBrush;         // The first index for into the brushes 
			int numOfLeafBrushes;  // The number of brushes for this leaf 
		}; 

		struct tBSPPlane
		{
			float vNormal[3];     // Plane normal. 
			float d;              // The plane distance from origin 
		};

		struct tBSPVisData
		{
			int numOfClusters;   // The number of clusters
			int bytesPerCluster; // Bytes (8 bits) in the cluster's bitset
			char *pBitsets;      // Array of bytes holding the cluster vis.
		}; 

		struct tBSPBrush 
		{
			int brushSide;           // The starting brush side for the brush 
			int numOfBrushSides;     // Number of brush sides for the brush
			int textureID;           // The texture index for the brush
		}; 

		struct tBSPBrushSide 
		{
			int plane;              // The plane index
			int textureID;          // The texture index
		}; 

		struct tBSPModel 
		{
			float min[3];           // The min position for the bounding box
			float max[3];           // The max position for the bounding box. 
			int faceIndex;          // The first face index in the model 
			int numOfFaces;         // The number of faces in the model 
			int brushIndex;         // The first brush index in the model 
			int numOfBrushes;       // The number brushes for the model
		}; 

		struct tBSPShader
		{
			char strName[64];     // The name of the shader file 
			int brushIndex;       // The brush index for this shader 
			int unknown;          // This is 99% of the time 5
		}; 

		struct tBSPLights
		{
			unsigned char ambient[3];     // This is the ambient color in RGB
			unsigned char directional[3]; // This is the directional color in RGB
			unsigned char direction[2];   // The direction of the light: [phi,theta] 
		}; 

		void loadTextures	(tBSPLump* l, ox::io::IReadFile* file);		// Load the textures
		void loadLightmaps  (tBSPLump* l, ox::io::IReadFile* file);      // Load the lightmaps
		void loadVerts      (tBSPLump* l, ox::io::IReadFile* file);		// Load the vertices
		void loadFaces      (tBSPLump* l, ox::io::IReadFile* file);		// Load the faces
		void loadPlanes     (tBSPLump* l, ox::io::IReadFile* file);		// Load the Planes of the BSP
		void loadNodes      (tBSPLump* l, ox::io::IReadFile* file);		// load the Nodes of the BSP
		void loadLeafs      (tBSPLump* l, ox::io::IReadFile* file);		// load the Leafs of the BSP
		void loadLeafFaces  (tBSPLump* l, ox::io::IReadFile* file);		// load the Faces of the Leafs of the BSP
		void loadVisData    (tBSPLump* l, ox::io::IReadFile* file);		// load the visibility data of the clusters
		void loadEntities   (tBSPLump* l, ox::io::IReadFile* file);		// load the entities
		void loadModels     (tBSPLump* l, ox::io::IReadFile* file);		// load the models
		void loadMeshVerts  (tBSPLump* l, ox::io::IReadFile* file);		// load the mesh vertices
		void loadBrushes    (tBSPLump* l, ox::io::IReadFile* file);		// load the brushes of the BSP
		void loadBrushSides (tBSPLump* l, ox::io::IReadFile* file);		// load the brushsides of the BSP
		void loadLeafBrushes(tBSPLump* l, ox::io::IReadFile* file);		// load the brushes of the leaf

		// second parameter i is the zero based index of the current face.
		void createCurvedSurface(SMeshBufferLightMap* meshBuffer, int i);
		float Blend( float s[3], float t[3], tBSPVertex *v[9], int offset);

		tBSPLump Lumps[kMaxLumps];

		tBSPTexture* Textures;
		int NumTextures;

		tBSPLightmap* LightMaps;
		int NumLightMaps;

		tBSPVertex* Vertices;
		int NumVertices;

		tBSPFace* Faces;
		int NumFaces;

		tBSPPlane* Planes;
		int NumPlanes;

		tBSPNode* Nodes;
		int NumNodes;

		tBSPLeaf* Leafs;
		int NumLeafs;

		int *LeafFaces;
		int NumLeafFaces;

		int *MeshVerts;           // The vertex offsets for a mesh 
		int NumMeshVerts;

		tBSPBrush* Brushes;
		int NumBrushes;

		ox::scene::SMesh Mesh;
		ox::video::IVideoDriver* Driver;
		ox::core::CString<char> LevelName;
		ox::io::IFileSystem* FileSystem; // needs because there are no file extenstions stored in .bsp files.
	};

} // end namespace scene
} // end namespace daisy

#endif

