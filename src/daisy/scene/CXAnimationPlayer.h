// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXAnimationPlayer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_X_ANIMATION_PLAYER_H_INCLUDED__
#define __C_X_ANIMATION_PLAYER_H_INCLUDED__

#include "ox/scene/IAnimatedMeshX.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CQuaternion.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshManipulator.h"
#include "ox/TArray.h"
#include "CXFileReader.h"
#include "ox/scene/SMesh.h"
#include "ox/scene/SMeshBuffer.h"

namespace ox { namespace video {} }
namespace ox { namespace scene {} }
namespace daisy
{
namespace video
{
using namespace ox::video;
}
namespace scene
{
using namespace ox::scene;

	class CXAnimationPlayer : public IAnimatedMeshX
	{
	public:

		//! constructor
		CXAnimationPlayer(CXFileReader* reader,
			ox::video::IVideoDriver* driver,
			IMeshManipulator* manip,
			const char* filename);

		//! destructor
		virtual ~CXAnimationPlayer();

		//! Gets the frame count of the animated mesh.
		virtual int getFrameCount();

		//! Returns the IMesh interface for a frame.
		virtual IMesh* getMesh(int frame, int detailLevel=255, int startFrameLoop=-1, int endFrameLoop=-1);

		//! Returns an axis aligned bounding box of the mesh.
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

		//! Returns a pointer to a transformation matrix
		virtual ox::core::CMatrix4* getMatrixOfJoint(int jointNumber, int frame);

		//! Gets joint count.
		virtual int getJointCount() const;

		//! Gets the name of a joint.
		virtual const char* getJointName(int number) const;

		//! Gets a joint number from its name
		virtual int getJointNumber(const char* name) const;

		//! Returns a pointer to list of points containing the skeleton.
		virtual const ox::TArray<ox::core::CVector3d<float> >* getDrawableSkeleton(int frame);

		//! Returns amount of animations in .X-file.
		virtual int getAnimationCount() const;

		//! Returns the name of an animation.
		virtual const char* getAnimationName(int idx) const;

		//! Sets an animation as animation to play back.
		virtual void setCurrentAnimation(int idx);

		//! Sets an animation as animation to play back.
		virtual bool setCurrentAnimation(const char* name);

	private:

		struct SWeightData
		{
			SWeightData() {};

			SWeightData(const SWeightData& other)
				: buffer(other.buffer), vertex(other.vertex), weight(other.weight)
			{}

			SWeightData(int b, int v, float w)
				: buffer(b), vertex(v), weight(w)
			{}

			int buffer;
			int vertex;
			float weight;

			bool operator <(const SWeightData& other) const
			{
				return (buffer < other.buffer ||
						(buffer == other.buffer &&
						vertex < other.vertex) ||
						(buffer == other.buffer &&
						vertex == other.vertex &&
						weight < other.weight));
			}
		};

		struct SVertexWeight
		{
			SVertexWeight() : weightCount(0) {}

			// weight per vertex, at maximum, 4 joints per vertex
			float weight[4];
			int joint[4];

			int weightCount;

			void add(float _weight, int _joint)
			{
				if (weightCount == 4)
					return;

				weight[weightCount] = _weight;
				joint[weightCount] = _joint;
				++weightCount;
			}
		};

		struct SJoint
		{
			int Parent; // index of parent
			ox::TArray<SWeightData> Weights;
			ox::core::CMatrix4 MatrixOffset;
			ox::core::CMatrix4 LocalMatrix;
			ox::core::CMatrix4 GlobalMatrix;
			ox::core::CMatrix4 AnimatedMatrix;
			ox::core::CMatrix4 LocalAnimatedMatrix;
			ox::core::CMatrix4 CombinedAnimationMatrix; // for faster computing
			ox::core::CString<char> Name;
			bool IsVirtualJoint; // for in .x file not weighted vertices
			bool WasAnimatedThisFrame; // used by animateSkeleton()
		};

		void createAnimationData();
		void createJointData(CXFileReader::SXFrame* f, int JointParent);
		void createMeshData();
		void addFrameToMesh(CXFileReader::SXFrame* frame);
		ox::video::SMaterial getMaterialFromXMaterial(CXFileReader::SXMaterial& xmat);
		void addFacesToBuffer(int meshbuffernr, CXFileReader::SXMesh& mesh, int matnr, CXFileReader::SXFrame* frame);
		ox::core::CString<char> getTextureFileName(ox::core::CString<char> texture);

		int getJointNumberFromName(ox::core::CString<char>& name);
		void addLineToSkeleton(ox::core::CVector3d<float> start, ox::core::CVector3d<float> end);

		//! prepares animation data which was read in from the .x file
		void prepareAnimationData();

		//! animates the skeleton based on the animation data
        void animateSkeleton();

		//! modifies the skin based on the animated skeleton
		void modifySkin();

		void updateBoundingBoxFromAnimation();

		void addVirtualWeight(int meshbuffernr, int vtxidx, CXFileReader::SXMesh& mesh,
			CXFileReader::SXFrame* frame);

		ox::video::IVideoDriver* Driver;
		ox::scene::SMesh OriginalMesh;
		ox::scene::SMesh *AnimatedMesh;
		CXFileReader* Reader;
		ox::core::CAabbox3d<float> Box;
		ox::core::CString<char> FileName;
		IMeshManipulator* Manipulator;
		ox::TArray<ox::core::CVector3d<float> > DebugSkeleton;
		bool IsAnimatedSkinnedMesh;

		ox::TArray<SJoint> Joints;

		float CurrentAnimationTime;
		float LastAnimationTime;
		int CurrentAnimationSet;
		float DebugSkeletonCrossSize;

		// one array of weights per mesh buffer
		ox::TArray< ox::TArray<SVertexWeight> > Weights;

		// data for animations

		struct SXAnimationTrack
		{
			int jointNr;
			int keyType; // 0=rotation, 1=scale, 2=position, 3=matrix

			ox::TArray<ox::core::CQuaternion> Quaternions;
			ox::TArray<ox::core::CVector3d<float> > Vectors;
			ox::TArray<ox::core::CMatrix4> Matrices;
			ox::TArray<float> Times;

			bool operator <(const SXAnimationTrack& other) const
			{
				if (jointNr != other.jointNr)
					return (jointNr < other.jointNr);

				return keyType > other.keyType;
			}
		};

		struct SXAnimationSet
		{
			ox::core::CString<char> AnimationName;
			ox::TArray<SXAnimationTrack> Animations;
		};

		ox::TArray<SXAnimationSet> AnimationSets;

		void addMatrix(ox::core::CMatrix4& m1, ox::core::CMatrix4& m2)
		{
			for (int i=0; i<16; ++i)
				m1.M[i] += m2.M[i];
		}
	};

} // end namespace scene
} // end namespace daisy

#endif
