// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGeometryCreator.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/video/ITexture.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/irrMath.h"
#include "ox/video/IImage.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/event/ILogger.h"
#include "ox/video/ColorPacking.h"
#include "CGeometryCreator.h"
#include "ox/scene/SAnimatedMesh.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/SMesh.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/video/Null/CImage.h"
#include <stdio.h>
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

// creates a hill plane
IAnimatedMesh* CGeometryCreator::createHillPlaneMesh(const ox::core::CDimension2d<float>& tileSize, const ox::core::CDimension2d<int>& tc,
	ox::video::SMaterial* material,	float hillHeight, const ox::core::CDimension2d<float>& ch,
	const ox::core::CDimension2d<float>& textureRepeatCount)
{
	ox::core::CDimension2d<int> tileCount = tc;
	tileCount.Height += 1;
	tileCount.Width += 1;

    ox::core::CDimension2d<float> countHills = ch;

	SMeshBuffer* buffer = new SMeshBuffer();
	SMesh* mesh = new SMesh();
	ox::video::S3DVertex vtx;
	vtx.Color = ox::video::SColor(255, 255, 255, 255);
	vtx.Normal.set(0,1,0);

	if (countHills.Width < 0.01f) countHills.Width = 1;
	if (countHills.Height < 0.01f) countHills.Height = 1;

	float halfX = (tileSize.Width * tileCount.Width) / 2;
	float halfY = (tileSize.Height * tileCount.Height) / 2;

	// create vertices

	int x = 0;
	int y = 0;

	ox::core::CDimension2d<float> tx;
	tx.Width = 1.0f / (tileCount.Width / textureRepeatCount.Width);
	tx.Height = 1.0f / (tileCount.Height / textureRepeatCount.Height);


	for (x=0; x<tileCount.Width; ++x)
		for (y=0; y<tileCount.Height; ++y)
		{
			vtx.Pos.set(tileSize.Width * x - halfX, 0, tileSize.Height * y - halfY);
			vtx.TCoords.set(-(float)x * tx.Width, (float)y * tx.Height);

			if (hillHeight)
				vtx.Pos.Y = (float)(sin(vtx.Pos.X * countHills.Width * ox::core::PI / halfX) *
					cos(vtx.Pos.Z * countHills.Height * ox::core::PI / halfY))
					*hillHeight;

			buffer->Vertices.push_back(vtx);
		}

	// create indices

	for (x=0; x<tileCount.Width-1; ++x)
		for (y=0; y<tileCount.Height-1; ++y)
		{
			int current = y*tileCount.Width + x;

			buffer->Indices.push_back(current);
			buffer->Indices.push_back(current + 1);
			buffer->Indices.push_back(current + tileCount.Width);

			buffer->Indices.push_back(current + 1);
			buffer->Indices.push_back(current + 1 + tileCount.Width);
			buffer->Indices.push_back(current + tileCount.Width);
		}

	// recalculate normals
	for (int i=0; i<(int)buffer->Indices.size(); i+=3)
	{
		ox::core::CPlane3d<float> p(
			buffer->Vertices[buffer->Indices[i+0]].Pos,
			buffer->Vertices[buffer->Indices[i+1]].Pos,
			buffer->Vertices[buffer->Indices[i+2]].Pos);
		p.Normal.normalize();

		buffer->Vertices[buffer->Indices[i+0]].Normal = p.Normal;
		buffer->Vertices[buffer->Indices[i+1]].Normal = p.Normal;
		buffer->Vertices[buffer->Indices[i+2]].Normal = p.Normal;
	}

	if (material)
		buffer->Material = *material;

	buffer->recalculateBoundingBox();

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	mesh->addMeshBuffer(buffer);
	mesh->recalculateBoundingBox();
	animatedMesh->addMesh(mesh);
	animatedMesh->recalculateBoundingBox();

	mesh->drop();
	buffer->drop();

	return animatedMesh;
}



IAnimatedMesh* CGeometryCreator::createTerrainMesh(ox::video::IImage* texture,
		ox::video::IImage* heightmap, const ox::core::CDimension2d<float>& stretchSize,
		float maxHeight, ox::video::IVideoDriver* driver,
		const ox::core::CDimension2d<int> maxVtxBlockSize,
		bool debugBorders)
{
	unsigned int tm = os::Timer::getTime()/1000;

	if (!texture || !heightmap)
		return 0;

	ox::video::SMaterial material;
	char textureName[64];
	char tmp[255];


	// debug border
	int borderSkip = debugBorders ? 0 : 1;

	ox::video::S3DVertex vtx;
	vtx.Color = ox::video::SColor(255, 255, 255, 255);

	SMesh* mesh = new SMesh();

	ox::core::CDimension2d<int> hMapSize= heightmap->getDimension();
	ox::core::CDimension2d<int> tMapSize= texture->getDimension();
	ox::core::CPosition2d<float> thRel((float)tMapSize.Width / (int)hMapSize.Width, (float)tMapSize.Height / (int)hMapSize.Height);
	ox::core::CPosition2d<int> processed(0,0);

	while (processed.Y<hMapSize.Height)
	{
		while(processed.X<hMapSize.Width)
		{
			ox::core::CDimension2d<int> blockSize = maxVtxBlockSize;
			if (processed.X + blockSize.Width > hMapSize.Width)
				blockSize.Width = hMapSize.Width - processed.X;
			if (processed.Y + blockSize.Height > hMapSize.Height)
				blockSize.Height = hMapSize.Height - processed.Y;

			SMeshBuffer* buffer = new SMeshBuffer();
			int x,y;

			// add vertices of vertex block
			for (y=0; y<blockSize.Height; ++y)
				for (x=0; x<blockSize.Width; ++x)
				{
					ox::video::SColor clr = heightmap->getPixel(x+processed.X, y+processed.Y);
					float height = ((clr.getRed() + clr.getGreen() + clr.getBlue()) / 3.0f)/255.0f * maxHeight;

					vtx.Pos.set((float)(x+processed.X) * stretchSize.Width,
						height, (float)(y+processed.Y) * stretchSize.Height);

					vtx.TCoords.set((float)(x+0.5f) / ((float)blockSize.Width),
						(float)(y+0.5f) / ((float)blockSize.Height));

					buffer->Vertices.push_back(vtx);
				}

            // add indices of vertex block
			for (y=0; y<blockSize.Height-1; ++y)
				for (x=0; x<blockSize.Width-1; ++x)
			{
				int c = (y*blockSize.Width) + x;

				buffer->Indices.push_back(c);
				buffer->Indices.push_back(c + blockSize.Width);
				buffer->Indices.push_back(c + 1);

				buffer->Indices.push_back(c + 1);
				buffer->Indices.push_back(c + blockSize.Width);
				buffer->Indices.push_back(c + 1 + blockSize.Width);
			}

			// recalculate normals
			for (int i=0; i<(int)buffer->Indices.size(); i+=3)
			{
				ox::core::CPlane3d<float> p(
					buffer->Vertices[buffer->Indices[i+0]].Pos,
					buffer->Vertices[buffer->Indices[i+1]].Pos,
					buffer->Vertices[buffer->Indices[i+2]].Pos);
				p.Normal.normalize();

				buffer->Vertices[buffer->Indices[i+0]].Normal = p.Normal;
				buffer->Vertices[buffer->Indices[i+1]].Normal = p.Normal;
				buffer->Vertices[buffer->Indices[i+2]].Normal = p.Normal;
			}

			if (buffer->Vertices.size())
			{
				// create texture for this block
				ox::video::IImage* img = new video::CImage(texture,
					ox::core::CPosition2d<int>((int)(processed.X*thRel.X), (int)(processed.Y*thRel.Y)),
					ox::core::CDimension2d<int>((int)(blockSize.Width*thRel.X), (int)(blockSize.Height*thRel.Y)));

				sprintf(textureName, "terrain%d_%d", tm, mesh->getMeshBufferCount());

				material.Texture1 = driver->addTexture(textureName, img);

				if (material.Texture1)
				{
					sprintf(tmp, "Generated terrain texture (%dx%d): %s",
						material.Texture1->getSize().Width,
						material.Texture1->getSize().Height,
						textureName);
					os::Printer::log(tmp, ox::event::ELL_INFORMATION);
				}
				else
					os::Printer::log("Could not create terrain texture.", textureName, ox::event::ELL_ERROR);

				buffer->Material = material;
				img->drop();
			}

			buffer->recalculateBoundingBox();
			mesh->addMeshBuffer(buffer);
			buffer->drop();

            // keep on processing
			processed.X += maxVtxBlockSize.Width - borderSkip;
		}

		// keep on processing
		processed.X = 0;
		processed.Y += maxVtxBlockSize.Height - borderSkip;
	}

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	mesh->recalculateBoundingBox();
	animatedMesh->addMesh(mesh);
	animatedMesh->recalculateBoundingBox();

	mesh->drop();

	return animatedMesh;
}


} // end namespace scene
} // end namespace daisy
