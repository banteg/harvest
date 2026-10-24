// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXAnimationPlayer.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CQuaternion.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "ox/algo/CArrayFunctions.h"
#include "CXAnimationPlayer.h"
#include "ox/scene/ISceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/os.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/IMeshManipulator.h"

#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CXAnimationPlayer::CXAnimationPlayer(CXFileReader* reader,
									 ox::video::IVideoDriver* driver,
									 IMeshManipulator* manip,
									 const char* filename)
: Reader(reader), Driver(driver), AnimatedMesh(0),
Manipulator(manip), CurrentAnimationTime(0.0f), LastAnimationTime(1.0f),
	CurrentAnimationSet(0), IsAnimatedSkinnedMesh(false), DebugSkeletonCrossSize(1.0f)
{
	FileName = filename;

	if (!Reader)
		return;

	if (Driver)
		Driver->grab();

	if (Manipulator)
		Manipulator->grab();

	Reader->grab();

	createAnimationData();
}



//! destructor
CXAnimationPlayer::~CXAnimationPlayer()
{
	if (Reader)
		Reader->drop();

	if (Driver)
		Driver->drop();

	if (AnimatedMesh)
		AnimatedMesh->drop();

	if (Manipulator)
		Manipulator->drop();
}



//! Gets the frame count of the animated mesh.
int CXAnimationPlayer::getFrameCount()
{
	return IsAnimatedSkinnedMesh ? (int)LastAnimationTime : 1;
}



//! Returns the IMesh interface for a frame.
IMesh* CXAnimationPlayer::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	if (!IsAnimatedSkinnedMesh)
		return &OriginalMesh;

	CurrentAnimationTime = (float)frame;
	animateSkeleton();
	modifySkin();
	updateBoundingBoxFromAnimation();
	return AnimatedMesh;
}



//! Returns an axis aligned bounding box of the mesh.
const ox::core::CAabbox3d<float>& CXAnimationPlayer::getBoundingBox() const
{
	return Box;
}



//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CXAnimationPlayer::getMeshType() const
{
	return EAMT_X;
}



void CXAnimationPlayer::createAnimationData()
{
	// get joints from x-file
	createJointData(&Reader->getRootFrame(), -1);

	createMeshData();

	if (IsAnimatedSkinnedMesh && AnimatedMesh)
	{
		animateSkeleton();
		modifySkin();
		updateBoundingBoxFromAnimation();
		DebugSkeletonCrossSize = AnimatedMesh->getBoundingBox().getExtent().X / 20.0f;
	}
	else
		DebugSkeletonCrossSize = OriginalMesh.getBoundingBox().getExtent().X / 20.0f;
}


void CXAnimationPlayer::createMeshData()
{
	// create mesh
	addFrameToMesh(&Reader->getRootFrame());

	// recalulate box
	OriginalMesh.recalculateBoundingBox();

	// sort weights in joints
	for (int j=0; j<(int)Joints.size(); ++j)
		ox::algo::sort(Joints[j].Weights.begin(), Joints[j].Weights.end());

	// copy mesh
	AnimatedMesh = Manipulator->createMeshCopy(&OriginalMesh);

	// create and link animation data
	prepareAnimationData();

	// first animation
	animateSkeleton();
	modifySkin();
}


void CXAnimationPlayer::addFrameToMesh(CXFileReader::SXFrame* frame)
{
	// go through all meshes
	for (unsigned int m=0; m<frame->Meshes.size(); ++m)
	{
		// create mesh buffer for every material
		if (frame->Meshes[m].MaterialList.Materials.empty())
			os::Printer::log("Mesh without material found in x file.", ox::event::ELL_WARNING);

		for (unsigned int mt=0; mt<frame->Meshes[m].MaterialList.Materials.size(); ++mt)
		{
			// create buffer
			SMeshBuffer *buf = new SMeshBuffer();
			OriginalMesh.addMeshBuffer(buf);
			buf->drop();

			// new weights buffer
			Weights.push_back( ox::TArray<SVertexWeight>() );

			// create material
			buf->Material = getMaterialFromXMaterial(
				frame->Meshes[m].MaterialList.Materials[mt]);

			// add all faces of this material
			addFacesToBuffer(OriginalMesh.MeshBuffers.size()-1,
				frame->Meshes[m], mt, frame);
			buf->recalculateBoundingBox();
		}
	}

	// add child frames
	for (unsigned int c=0; c<frame->ChildFrames.size(); ++c)
		addFrameToMesh(&frame->ChildFrames[c]);
}



ox::video::SMaterial CXAnimationPlayer::getMaterialFromXMaterial(CXFileReader::SXMaterial& xmat)
{
	ox::video::SMaterial mat;
	mat.EmissiveColor = ox::video::SColor((int)(xmat.Emissive.a*255.0f), (int)(xmat.Emissive.r*255.0f), (int)(xmat.Emissive.g*255.0f), (int)(xmat.Emissive.b*255.0f));
	mat.DiffuseColor = ox::video::SColor((int)(xmat.FaceColor.a*255.0f), (int)(xmat.FaceColor.r*255.0f), (int)(xmat.FaceColor.g*255.0f), (int)(xmat.FaceColor.b*255.0f));
	mat.SpecularColor = ox::video::SColor((int)(xmat.Specular.a*255.0f), (int)(xmat.Specular.r*255.0f), (int)(xmat.Specular.g*255.0f), (int)(xmat.Specular.b*255.0f));
	mat.Shininess = xmat.Power;

	//if (mat.Shininess > ox::core::ROUNDING_ERROR)
	//	mat. TODO: enable specular highlights

	if (xmat.TextureFileName.size() != 0)
	{
		mat.Texture1 = Driver->getTexture(getTextureFileName(xmat.TextureFileName).c_str());
		if (mat.Texture1 == 0)
			mat.Texture1 = Driver->getTexture(xmat.TextureFileName.c_str());
	}

	return mat;
}



} }
namespace ox { namespace video {
inline bool operator == (const ox::video::S3DVertex& a, const ox::video::S3DVertex& b)
{
	return (a.Pos == b.Pos && a.Normal == b.Normal &&
				b.Color.color == a.Color.color && a.TCoords == b.TCoords);
}
} }
namespace daisy { namespace scene {
void CXAnimationPlayer::addFacesToBuffer(int meshbuffernr, CXFileReader::SXMesh& mesh, int matnr, CXFileReader::SXFrame* frame)
{
	ox::scene::SMeshBuffer* buf = (SMeshBuffer*)OriginalMesh.MeshBuffers[meshbuffernr];

	int vcnt = mesh.Vertices.size();
	int tcnt = mesh.TextureCoords.size();
	int ncnt = mesh.Normals.size();
	int fcnt = mesh.Indices.size();

	// precompute which joint belongs to which weight array
	ox::TArray< int > jointNumberWeightNumberMap;
	for (int w=0; w<(int)mesh.SkinWeights.size(); ++w)
	{
		int jnr = getJointNumberFromName(mesh.SkinWeights[w].TransformNodeName);
		if (jnr == -1)
			os::Printer::log("Unknown Joint referenced in x file",
				mesh.SkinWeights[w].TransformNodeName.c_str(), ox::event::ELL_WARNING);
		else
		{
			Joints[jnr].MatrixOffset = mesh.SkinWeights[w].MatrixOffset;
			IsAnimatedSkinnedMesh = true;
		}

		jointNumberWeightNumberMap.push_back(jnr);
	}


	ox::video::S3DVertex v;
	v.Color = 0xFFFFFFFF;

	// add only those with material matnr
	for (int i=0; i<(int)mesh.MaterialList.FaceIndices.size(); ++i)
		if (mesh.MaterialList.FaceIndices[i] == matnr)
		{
			// add face number i
			for (int f=0; f<3; ++f)
			{
				int idxidx = i*3+f;
				int idx = mesh.Indices[idxidx];

				v.Pos = mesh.Vertices[idx];

				if (tcnt) v.TCoords = mesh.TextureCoords[idx];
				if (ncnt) v.Normal = mesh.Normals[mesh.NormalIndices[idxidx]];

				int nidx = ox::algo::linearSearchPos(buf->Vertices.begin(), buf->Vertices.end(), v);
				bool alreadyIn = (nidx != -1);

				if (!alreadyIn)
				{
					nidx = buf->Vertices.size();
					buf->Indices.push_back(nidx);
					buf->Vertices.push_back(v);
					Weights[meshbuffernr].push_back(SVertexWeight());
				}
				else
					buf->Indices.push_back(nidx);

				bool isWeighted = alreadyIn;

				// add weight data of this vertex to all joints
				if (!alreadyIn)
				for (int w=0; w<(int)mesh.SkinWeights.size(); ++w)
				{
					int vertexinweights = ox::algo::binarySearchPos(mesh.SkinWeights[w].Weights.begin(), mesh.SkinWeights[w].Weights.end(), CXFileReader::SXWeight(idx));

					if (vertexinweights != -1)
					{
						int jnr = jointNumberWeightNumberMap[w];

                        if (jnr != -1)
						{
							// weight per joint
							Joints[jnr].Weights.push_back(SWeightData(
								meshbuffernr,
								nidx,
								mesh.SkinWeights[w].Weights[vertexinweights].Weight));

							// weight per vertex
							SVertexWeight& weight = Weights[meshbuffernr][nidx];
							weight.add(mesh.SkinWeights[w].Weights[vertexinweights].Weight, jnr);

							isWeighted = true;
						}
					}
				} // end for all weights

				// if this vertex does not have a weight, create a virtual new
				// joint and attach it to this one
				if (!isWeighted)
					addVirtualWeight(meshbuffernr, nidx, mesh, frame);

			} // end for all faces

		} // end for all materials

	// generate missing normals
	if (!ncnt)
	{
		ox::video::S3DVertex* v = &buf->Vertices[0];
		fcnt = buf->Indices.size();
		for (int u=0; u<fcnt; u+=3)
		{
			ox::core::CPlane3d<float> p(v[buf->Indices[u+0]].Pos,
				v[buf->Indices[u+1]].Pos, v[buf->Indices[u+2]].Pos);
			p.Normal.normalize();
			v[buf->Indices[u+0]].Normal = p.Normal;
			v[buf->Indices[u+1]].Normal = p.Normal;
			v[buf->Indices[u+2]].Normal = p.Normal;
		}
	}

	// transform vertices and normals
	/*ox::core::CMatrix4 mat = frame->LocalMatrix;
	mat.makeInverse();

	vcnt = buf->Vertices.size();
	for (int u=0; u<vcnt; ++u)
	{
		mat.transformVect(buf->Vertices[u].Pos);
		mat.inverseRotateVect(buf->Vertices[u].Normal);
	}*/
}


void CXAnimationPlayer::addVirtualWeight(int meshbuffernr, int vtxidx,
										CXFileReader::SXMesh& mesh,
										CXFileReader::SXFrame* frame)
{
	// find original joint of vertex

	int jnr = getJointNumberFromName(frame->Name);
	if (jnr == -1)
		return;

	// weight per joint
	Joints[jnr].Weights.push_back(SWeightData(
		meshbuffernr,
		vtxidx,
		1.0f));

	// weight per vertex
	SVertexWeight& weight = Weights[meshbuffernr][vtxidx];
	weight.add(1.0f, jnr);
}



int CXAnimationPlayer::getJointNumberFromName(ox::core::CString<char>& name)
{
	for (int i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].Name == name)
			return i;

	return -1;
}



ox::core::CString<char> CXAnimationPlayer::getTextureFileName(ox::core::CString<char> texture)
{
	int idx = -1;
	idx = FileName.findLast('/');

    if (idx == -1)
		idx = FileName.findLast('\\');

	if (idx == -1)
		return ox::core::CString<char>();

	ox::core::CString<char> p = FileName.subString(0, idx+1);
	p.append(texture);
	return p;
}



void CXAnimationPlayer::createJointData(CXFileReader::SXFrame* f, int JointParent)
{
	// add joint

	int index = Joints.size();
	Joints.push_back(SJoint());
	SJoint& j = Joints.back();

	j.Parent = JointParent;
	j.GlobalMatrix = f->GlobalMatrix;
	j.LocalMatrix = f->LocalMatrix;
	j.AnimatedMatrix = j.GlobalMatrix;
	j.LocalAnimatedMatrix = j.LocalMatrix;
	j.CombinedAnimationMatrix = j.AnimatedMatrix * j.MatrixOffset;
	j.IsVirtualJoint = false;

	j.Name = f->Name;

	// add all children
	for (int i=0; i<(int)f->ChildFrames.size(); ++i)
		createJointData(&f->ChildFrames[i], index);
}


//! Returns a pointer to a transformation matrix
ox::core::CMatrix4* CXAnimationPlayer::getMatrixOfJoint(int jointNumber, int frame)
{
	if (jointNumber < 0 || jointNumber >= (int)Joints.size())
		return 0;

	return &Joints[jointNumber].CombinedAnimationMatrix;
}


//! Gets joint count.
int CXAnimationPlayer::getJointCount() const
{
	return Joints.size();
}


//! Gets the name of a joint.
const char* CXAnimationPlayer::getJointName(int number) const
{
	if (number<0 || number>=(int)Joints.size())
		return 0;

	return Joints[number].Name.c_str();
}


//! Gets a joint number from its name
int CXAnimationPlayer::getJointNumber(const char* name) const
{
	for (int i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].Name == name)
			return i;

	return -1;
}


//! Returns a pointer to list of points containing the skeleton.
const ox::TArray<ox::core::CVector3d<float> >* CXAnimationPlayer::getDrawableSkeleton(int frame)
{
	DebugSkeleton.clear();

	float k = DebugSkeletonCrossSize;
	float p = 0.0f;

	for (int i=0; i<(int)Joints.size(); ++i)
	{
		ox::core::CVector3d<float> start(p,p,p);
		ox::core::CVector3d<float> end(p,p,p);

		Joints[i].AnimatedMatrix.transformVect(start);

		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,k,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,-k,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(k,0,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(-k,0,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,0,k));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,0,-k));

		if (Joints[i].Parent != -1)
		{
			Joints[Joints[i].Parent].AnimatedMatrix.transformVect(end);

			DebugSkeleton.push_back(end);
			DebugSkeleton.push_back(start);
		}
	}

	return &DebugSkeleton;
}


//! animates the skeleton based on the animation data
inline ox::core::CMatrix4 interpolate(const ox::core::CMatrix4& self, const ox::core::CMatrix4& b, float time, int pad = 0)
{
	float ntime = 1.0f - time;
	ox::core::CMatrix4 mat;
	if (pad) { mat.M[0] = 0.0f; mat.M[1] = 0.0f; mat.M[2] = 0.0f; mat.M[3] = 0.0f; mat.M[4] = 0.0f; mat.M[5] = 0.0f; mat.M[6] = 0.0f; mat.M[7] = 0.0f; mat.M[8] = 0.0f; }
	for (int i=0; i<16; ++i)
		mat.M[i] = self.M[i]*ntime + b.M[i]*time;

	return mat;
}

void CXAnimationPlayer::animateSkeleton()
{
	int i;

	if (!AnimationSets.empty())
	{

		// reset joints
		for (int jii=0; jii<(int)Joints.size(); ++jii)
		{
			Joints[jii].LocalAnimatedMatrix.makeIdentity();
			Joints[jii].WasAnimatedThisFrame = false;
		}

		SXAnimationSet& currentSet = AnimationSets[CurrentAnimationSet];

		// go through all animation tracks
		for (i=0; i<(int)currentSet.Animations.size(); ++i)
		{
			SJoint& joint = Joints[currentSet.Animations[i].jointNr];

			// find indexes to interpolate between
			int idx1 = -1;
			int idx2 = -1;

			for (int t=0; t<(int)currentSet.Animations[i].Times.size()-1; ++t)
				if (currentSet.Animations[i].Times[t] <= CurrentAnimationTime &&
					currentSet.Animations[i].Times[t+1] >= CurrentAnimationTime)
				{
					idx1 = t;
					idx2 = (t+1) % currentSet.Animations[i].Times.size();
					break;
				}

			if (idx1 == -1 || idx2 == -1)
				continue;

			// calculate interpolation factor
			float factor = (CurrentAnimationTime - currentSet.Animations[i].Times[idx1]) /
				(currentSet.Animations[i].Times[idx2] - currentSet.Animations[i].Times[idx1]);

			// animate it
			switch(currentSet.Animations[i].keyType)
			{
			case 0: // rotation
				{
					// with this code, rotations are not 100% ok, they are
					// mirrored.
					ox::core::CQuaternion q;
					q.slerp(currentSet.Animations[i].Quaternions[idx1],
							currentSet.Animations[i].Quaternions[idx2],
							factor);

					joint.LocalAnimatedMatrix *= q.getMatrix();
					joint.WasAnimatedThisFrame = true;
				}
				break;
			case 1: // scale
				{
					ox::core::CMatrix4 mat1, mat2;
					mat1.setScale(currentSet.Animations[i].Vectors[idx1]);
					mat2.setScale(currentSet.Animations[i].Vectors[idx2]);

					joint.LocalAnimatedMatrix *= interpolate(mat1, mat2, factor);
					joint.WasAnimatedThisFrame = true;
				}
				break;
			case 2: // position
				{
					ox::core::CMatrix4 mat1, mat2;
					mat1.setTranslation(currentSet.Animations[i].Vectors[idx1]);
					mat2.setTranslation(currentSet.Animations[i].Vectors[idx2]);

					joint.LocalAnimatedMatrix *= interpolate(mat1, mat2, factor);
					joint.WasAnimatedThisFrame = true;
				}
				break;
			case 4:
			case 3: // matrix
				{
					joint.LocalAnimatedMatrix = interpolate(currentSet.Animations[i].Matrices[idx1], currentSet.Animations[i].Matrices[idx2], factor);

					joint.WasAnimatedThisFrame = true;
				}
				break;
			}

		}
	}


	// update all joints
	for (int ji=0; ji<(int)Joints.size(); ++ji)
	{
		if (!Joints[ji].WasAnimatedThisFrame)
			Joints[ji].LocalAnimatedMatrix = Joints[ji].LocalMatrix;

        Joints[ji].AnimatedMatrix = Joints[ji].LocalAnimatedMatrix;

		if (Joints[ji].Parent != -1)
			Joints[ji].AnimatedMatrix = Joints[Joints[ji].Parent].AnimatedMatrix * Joints[ji].AnimatedMatrix;

		Joints[ji].CombinedAnimationMatrix = Joints[ji].AnimatedMatrix *
			Joints[ji].MatrixOffset;
	}
}


//! modifies the skin based on the animated skeleton
void CXAnimationPlayer::modifySkin()
{
	// set animated vertices to zero
	for (int k=0; k<(int)Joints.size(); ++k)
	{
		for (int w=0; w<(int)Joints[k].Weights.size(); ++w)
		{
			SWeightData& wd = Joints[k].Weights[w];

#ifdef _XREADER_DEBUG
			if (wd.buffer >= AnimatedMesh->getMeshBufferCount() ||
				wd.vertex >= AnimatedMesh->getMeshBuffer(wd.buffer)->getVertexCount())
				os::Printer::log("CXAnimationPlayer: Invalid Weights", ox::event::ELL_INFORMATION);
#endif

			ox::video::S3DVertex* nv = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(wd.buffer)->getVertices();
            nv[wd.vertex].Pos.set(0,0,0);
		}
	}


#ifdef _XREADER_DEBUG
	bool somethingIsWrong = false;

	// check if all vertices are set to zero
	for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
	{
		ox::video::S3DVertex* v = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(mb)->getVertices();
		int c = AnimatedMesh->getMeshBuffer(mb)->getVertexCount();
		for (int vt=0; vt<c; ++vt)
			if (v[vt].Pos != ox::core::CVector3d<float>(0,0,0))
			{
				char tmp[255];
				sprintf(tmp, "CXAnimationPlayer: Vertex not null. Buf:%d Vtx:%d", mb, vt);
				// this can happen if something is wrong with the vertex weights,
				// not all vertices have a weight attached.
				os::Printer::log(tmp, ox::event::ELL_INFORMATION);
				somethingIsWrong = true;
			}
	}

	if (somethingIsWrong)
	{
		// write out mesh informations
		char tmp[255];
		sprintf(tmp, "CXAnimationPlayer: Meshbuffers:%d", AnimatedMesh->getMeshBufferCount());
		os::Printer::log(tmp, ox::event::ELL_INFORMATION);

		for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
		{
			sprintf(tmp, "CXAnimationPlayer: Meshbuffer #%d: %d vertices", mb, AnimatedMesh->getMeshBuffer(mb)->getVertexCount());
			os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		}
	}
#endif

	// transform vertices
	for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
	{
		ox::video::S3DVertex* av = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(mb)->getVertices();
		ox::video::S3DVertex* ov = (ox::video::S3DVertex*)OriginalMesh.getMeshBuffer(mb)->getVertices();
		int c = AnimatedMesh->getMeshBuffer(mb)->getVertexCount();
		for (int vt=0; vt<c; ++vt)
		{
			ox::core::CVector3d<float> vtmp;
			ox::core::CVector3d<float> orig = ov[vt].Pos;
			SVertexWeight& weight = Weights[mb][vt];

			av[vt].Pos = ox::core::CVector3d<float>(0,0,0);

			/*if (weight.weightCount == 0)
			{
				av[vt].Pos = ov[vt].Pos;
			}
			else*/
			for (int w=0; w<weight.weightCount; ++w)
			{
				Joints[weight.joint[w]].CombinedAnimationMatrix.transformVect(
					orig, vtmp);

				/*ox::core::CMatrix4 mat;
				mat = Joints[weight.joint[w]].GlobalMatrix;
				mat *= Joints[weight.joint[w]].MatrixOffset;
				mat.transformVect(orig, vtmp);*/

				vtmp *= weight.weight[w];
				av[vt].Pos += vtmp;
			}


		}
	}

	// transform vertices
	/*ox::core::CVector3d<float> vtmp;
	for (int k=0; k<(int)Joints.size(); ++k)
	{

		for (int w=0; w<(int)Joints[k].Weights.size(); ++w)
		{
			SWeightData& wd = Joints[k].Weights[w];
			ox::video::S3DVertex* nv = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(wd.buffer)->getVertices();
			ox::video::S3DVertex* ov = (ox::video::S3DVertex*)OriginalMesh.getMeshBuffer(	wd.buffer)->getVertices();

			ox::core::CMatrix4 m = Joints[k].XFrame->GlobalMatrix;
			m *= Joints[k].MatrixOffset;

			m.transformVect(ov[wd.vertex].Pos, vtmp);

			vtmp *= wd.weight;
			nv[wd.vertex].Pos += vtmp;

		}
	}*/

}


//! prepares animation data which was read in from the .x file
void CXAnimationPlayer::prepareAnimationData()
{
    int animationSetCount = Reader->getAnimationSetCount();
	for (int i=0; i<animationSetCount; ++i)
	{
		AnimationSets.push_back(SXAnimationSet());
		SXAnimationSet& mySet = AnimationSets.back();

		CXFileReader::SXAnimationSet& readerSet = Reader->getAnimationSet(i);
		mySet.AnimationName = readerSet.AnimationName;

		// trough all animations
		for (int a=0; a<(int)readerSet.Animations.size(); ++a)
		{
			// trough all keys
			for (int k=0; k<(int)readerSet.Animations[a].Keys.size(); ++k)
			{
				// link with joint
				int jntnr = getJointNumberFromName(readerSet.Animations[a].FrameName);

				if (jntnr == -1)
				{
					os::Printer::log(
						"CXAnimationPlayer: Animationtrack without corresponding joint found",
						readerSet.Animations[a].FrameName.c_str(), ox::event::ELL_INFORMATION);
					continue;
				}

				// add new track
				mySet.Animations.push_back(SXAnimationTrack());
				SXAnimationTrack& myTrack = mySet.Animations.back();
				myTrack.jointNr = jntnr;
				IsAnimatedSkinnedMesh = true;

				// copy track
				int keyCount = (int)readerSet.Animations[a].Keys[k].numberOfKeys;
				int type = readerSet.Animations[a].Keys[k].keyType;
				int i;
				myTrack.keyType = type;

		#ifdef _XREADER_DEBUG
				char tmp[255];
				sprintf(tmp, "adding track %s with %d keys, type:%d", readerSet.Animations[a].FrameName.c_str(), keyCount, type);
				os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		#endif

				switch(type)
				{
				case 0: // quaternion
					for (i=0; i<keyCount; ++i)
						myTrack.Quaternions.push_back(readerSet.Animations[a].Keys[k].getQuaternion(i));
					break;
				case 1: // scale
				case 2: // position
					for (i=0; i<keyCount; ++i)
						myTrack.Vectors.push_back(readerSet.Animations[a].Keys[k].getVector(i));
					break;
				case 4:
				case 3: // matrix
					for (i=0; i<keyCount; ++i)
						myTrack.Matrices.push_back(readerSet.Animations[a].Keys[k].getMatrix(i));
					break;
				}

				// copy times
				for (i=0; i<keyCount; ++i)
					myTrack.Times.push_back((float)readerSet.Animations[a].Keys[k].time[i]);

				if (myTrack.Times.back() > LastAnimationTime)
					LastAnimationTime = myTrack.Times.back();
			}

		}

		// sort animation tracks
		ox::algo::sort(mySet.Animations.begin(), mySet.Animations.end());
	}
}

void CXAnimationPlayer::updateBoundingBoxFromAnimation()
{
	if (!Joints.size())
		return;

	bool first = true;

	for (int i=1; i<(int)Joints.size(); ++i)
		if (!Joints[i].Weights.empty())
		{
			ox::core::CVector3d<float> p(0,0,0);
			Joints[i].AnimatedMatrix.transformVect(p);

			if (first)
				Box.reset(p);
			else
				Box.addInternalPoint(p);

			first = false;
		}

	AnimatedMesh->BoundingBox = Box;

}

//! Returns amount of animations in .X-file.
int CXAnimationPlayer::getAnimationCount() const
{
	return AnimationSets.size();
}

//! Returns the name of an animation.
const char* CXAnimationPlayer::getAnimationName(int idx) const
{
	if (idx < 0 || idx >= (int)AnimationSets.size())
		return 0;

	return AnimationSets[idx].AnimationName.c_str();
}


//! Sets an animation as animation to play back.
void CXAnimationPlayer::setCurrentAnimation(int idx)
{
	if (idx < 0 || idx >= (int)AnimationSets.size())
		return;

	CurrentAnimationSet = idx;
}


//! Sets an animation as animation to play back.
bool CXAnimationPlayer::setCurrentAnimation(const char* name)
{
	for (int i=0; i<(int)AnimationSets.size(); ++i)
		if (AnimationSets[i].AnimationName == name)
		{
			CurrentAnimationSet = i;
			return true;
		}

	return false;
}


} // end namespace scene
} // end namespace daisy
