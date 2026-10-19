// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBspTree.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/event/ILogger.h"
#include "ox/video/S3DVertex.h"
#include <stdio.h>
#include "daisy/os.h"

#ifdef _DEBUG
#include <assert.h>
#endif

#include "CBspTree.h"
#include "ox/core/CLine3d.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


CBspTree::CBspTree(const ox::scene::IMeshBuffer* array, unsigned int bufferCount)
{
	createStats(array, bufferCount);
	createRoot(array, bufferCount);
	createBspTree(root);
	createRenderBuffer();

	// output creation stats.

#ifdef _DEBUG
	char tmp[255];
	os::Printer::log("/* Bsp Tree Creation stats block--------------", ox::event::ELL_INFORMATION);

	sprintf(tmp, " * LeafCounts: %d", stats.leafCounts);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * PolySplits: %d", stats.polySplits);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * NodeCount: %d", stats.nodeCount);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * Total Index count: %d", stats.totalIndexCount);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * Total Vertex count: %d", stats.totalVertexCount);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * Original total Index count: %d", stats.originalTotalIndexCount);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	sprintf(tmp, " * Original total vertex count: %d", stats.originalTotalVertexCount);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	os::Printer::log(" *--------------------------------------------*/", ox::event::ELL_INFORMATION);
#endif
}



CBspTree::~CBspTree()
{
	deleteTree(&root);
	deleteStats();

	delete [] renderBuffer;
}




void CBspTree::createBspTree(BspNode& node)
{
	int vertexCount = createBoundingBox(&node.polys[0], node.polys.size(), node.bbox);

	bool found = true;

	if (vertexCount < 50) // stoppt den vorgang bei 50 vertizen.
		found = false;
	
	if (found)
		found = findSplitter(&node.polys[0], node.polys.size(), node.plane);

	if (!found)	// es wurde keine ebene gefunden, daher ist es ein leaf
	{
		node.isLeaf = true;
		node.back = 0;
		node.front = 0;

		++stats.leafCounts;

		countPolyIndizes(node);
		countPolyVertizes(node);

		stats.boxes.push_back(node.bbox);

		return;
	}

	// es ist eine node, wir machen kinder.
	node.front = new BspNode();
	node.back = new BspNode();
	stats.nodeCount += 2;

	// polys in front oder back kopieren
	sortPolys(node);

	// polygone löschen
	node.polys.clear();

	// mit kindern das gleiche machen
	createBspTree(*node.front);
	createBspTree(*node.back);
}




void CBspTree::sortPolys(const BspNode& node)
{
	for (unsigned int i=0; i<node.polys.size(); ++i)
	{
		ox::core::EIntersectionRelation3D bpc = classifyPoly(node.plane, node.polys[i]);

		switch(bpc)
		{
		case ox::core::ISREL3D_FRONT:
			node.front->polys.push_back(node.polys[i]);
			break;
		case ox::core::ISREL3D_BACK:
			node.back->polys.push_back(node.polys[i]);
			break;
		case ox::core::ISREL3D_SPANNING: // muss geteilt werden
			{
				BspPoly newFront, newBack;
				
				splitPoly(node.polys[i], node.plane, newFront, newBack);

				node.front->polys.push_back(newFront);
				node.back->polys.push_back(newBack);
			}
			break;
		case ox::core::ISREL3D_PLANAR: // schauen in welche richtung es schaut
			{
				if (node.polys[i].plane.Normal.dotProduct(node.plane.Normal) >= 0.0f)
					node.front->polys.push_back(node.polys[i]);
				else
					node.back->polys.push_back(node.polys[i]);
			}
			break;
		}
	}

}


// liefert die anzahl der gesamten vertices im node zurück
int CBspTree::createBoundingBox(const BspPoly* poly, unsigned int polyCount, ox::core::CAabbox3d<float>& outBox)
{
	int totalCount = 0;
	outBox.reset(poly->vertices[0].Pos);

	for (unsigned int i = 0; i<polyCount; ++i)
	{
		totalCount += poly[i].vertices.size();

		for (unsigned int j = 0; j<poly[i].vertices.size(); ++j)
			outBox.addInternalPoint(poly[i].vertices[j].Pos);
	}

	return totalCount;
}




bool CBspTree::findSplitter(BspPoly* poly, unsigned int polyCount, ox::core::CPlane3d<float>& outSplitter)
{
	BspPoly* bestSplitter = 0;
	int bestScore = 0xfffffff;

	for (unsigned int i=0; i<polyCount; ++i)
	{
		unsigned int frontCount = 0;
		unsigned int backCount = 0;
		unsigned int planarCount = 0;
		unsigned int splitCount = 0;

		if (!poly[i].wasSplitter)
		{
			for (unsigned int j=0; j<polyCount; ++j)
			{
				if (i!=j)
				{
					ox::core::EIntersectionRelation3D bpc = classifyPoly(poly[i].plane, poly[j]);

					switch(bpc)
					{
					case ox::core::ISREL3D_FRONT: ++frontCount; break;
					case ox::core::ISREL3D_BACK: ++backCount; break;
					case ox::core::ISREL3D_PLANAR: ++planarCount; break;
					case ox::core::ISREL3D_SPANNING: ++splitCount; break;
					}
				}
			}

			int score = abs(frontCount - backCount) + (splitCount*3) + planarCount;
			//unsigned int score = abs(frontCount - backCount) + (splitCount*3) + planarCount;

			if (score < bestScore)
			{
				if ( ((frontCount > 0) && (backCount > 0)) || (splitCount > 0)) // Ist die Polyliste nicht konvex?
				{
					bestScore = score;
					bestSplitter = &poly[i];
				} 
			}
		}
	}

	if (!bestSplitter)
		return false;

	bestSplitter->wasSplitter = true;
	outSplitter = bestSplitter->plane;

	return true;
}




void CBspTree::render(const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos, bool aabbRendering)
{
	for (unsigned int i=0; i<stats.materialCount; ++i)
	{
		renderBuffer[i].vertices.clear();
		renderBuffer[i].indices.clear();
	}

	stats.renderCullings = 0;
	stats.renderedIndices = 0;

	if (!aabbRendering)
		render(root, camArea, camPos);
	else
		renderBoxCulling(root, camArea, camPos);
}




void CBspTree::render(BspNode& node, const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos)
{
	++stats.renderCullings;

	if (cullBoxAgainsView(node.bbox, camArea) == BCR_COMPLETE_OUTSIDE)
      return;

	if (node.isLeaf)
	{
		renderLeaf(node);
		return;
	}

	if (node.plane.classifyPointRelation(camPos) == ox::core::ISREL3D_BACK)
	{
		render(*node.front, camArea, camPos);
		render(*node.back, camArea, camPos);
	}
	else
	{
		render(*node.back, camArea, camPos);
		render(*node.front, camArea, camPos);
	}
}




void CBspTree::renderBoxCulling(BspNode& node, const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos)
{
	++stats.renderCullings;

	if (!node.bbox.intersectsWithBox(camArea->getBoundingBox()))
     return;

	if (node.isLeaf)
	{
		renderLeaf(node);
		return;
	}

	renderBoxCulling(*node.front, camArea, camPos);
	renderBoxCulling(*node.back, camArea, camPos);
}




void CBspTree::renderLeaf(BspNode& node)
{
	for (unsigned int i=0; i<node.polys.size(); ++i)
	{
		int material = node.polys[i].material;
		RenderBuffer* buffer = &renderBuffer[material];

		int start = buffer->vertices.size();

		// add vertices

		int vtxCount = node.polys[i].vertices.size();

		for (int v=0; v<vtxCount; ++v)
			buffer->vertices.push_back(node.polys[i].vertices[v]);

		// add indices

		int idxCount = node.polys[i].indices.size();
		
		for (int j=0; j<idxCount; ++j)
			buffer->indices.push_back(node.polys[i].indices[j] + start);

		// update stats

		stats.renderedIndices += idxCount;
	}
}




CBspTree::BoxCullingResult CBspTree::cullBoxAgainsView(const ox::core::CAabbox3d<float>& box, const ox::scene::SViewFrustrum* camArea)
{
	bool clipped = false;

	for (int i=0; i<6; ++i)
		switch(box.classifyPlaneRelation(camArea->planes[i]))
		{
		case ox::core::ISREL3D_FRONT:
			return BCR_COMPLETE_OUTSIDE;
		case ox::core::ISREL3D_CLIPPED:
			clipped = true;
			break;
		}

	if (clipped)
		return BCP_CLIPPED;

	return BCR_COMPLETE_INSIDE;
}




ox::core::EIntersectionRelation3D CBspTree::classifyPoly(const ox::core::CPlane3d<float>& plane, const BspPoly& poly)
{
	unsigned int countFront = 0;
	unsigned int countBack = 0;
	unsigned int countPlanar = 0;

	for (unsigned int i=0; i < poly.vertices.size(); ++i)
	{
		ox::core::EIntersectionRelation3D ir3d = plane.classifyPointRelation(poly.vertices[i].Pos);

		switch(ir3d)
		{
		case ox::core::ISREL3D_FRONT: ++countFront; break;
		case ox::core::ISREL3D_BACK: ++countBack; break;
		case ox::core::ISREL3D_PLANAR: ++countPlanar; ++countFront; ++countBack; break;
		}
	}

	if (countPlanar == poly.vertices.size()) // Alle Verts des Polys sind planar
		return ox::core::ISREL3D_PLANAR; 

	if (countFront == poly.vertices.size()) // Alle Verts des Polys liegen vor der Ebene
		return ox::core::ISREL3D_FRONT; 

	if (countBack == poly.vertices.size()) // Alle Verts des Polys liegen hinter der Ebene
		return ox::core::ISREL3D_BACK; 

	return ox::core::ISREL3D_SPANNING;
}




ox::core::CPlane3d<float> CBspTree::planeFromPoly(const BspPoly& poly)
{
	return ox::core::CPlane3d<float>(	poly.vertices[poly.indices[0]].Pos,
									poly.vertices[poly.indices[1]].Pos,
									poly.vertices[poly.indices[2]].Pos);
}




void CBspTree::deleteTree(BspNode *pNode)
{
	if (pNode->front)
	{
		deleteTree(pNode->front);
		delete pNode->front;
	}

	if (pNode->back)
	{
		deleteTree(pNode->back);
		delete pNode->back;
	}
}
 



void CBspTree::createRoot(const ox::scene::IMeshBuffer* array, unsigned int arraySize)
{
	// Behandelt jeweils drei indizes als ein polygon.
	// TODO: Die Routine sollte zusammenliegende Dreiecke in einer Ebene
	// als ein einzelnes polygon ansehen.

	root.isLeaf = false;

	//for (unsigned int i=0; i<arraySize; ++i)
	//	root.countPolys += array[i].getIndexCount() / 3;
	
	//root.polys.resize(root.countPolys);

	root.front = 0;
	root.back = 0;

	for (unsigned int i=0; i<arraySize; ++i)
	{
		const ox::video::S3DVertex* vertices = (ox::video::S3DVertex*)array[i].getVertices();

		stats.materialMap[i] = array[i].getMaterial();

		const unsigned short* indices = array[i].getIndices();

		for (int index = 0; index < array[i].getIndexCount(); index+=3)
		{
			BspPoly poly;
			poly.vertices.push_back( vertices[indices[index]] );
			poly.vertices.push_back( vertices[indices[index+1]] );
			poly.vertices.push_back( vertices[indices[index+2]] );

			poly.indices.push_back( 0 );
			poly.indices.push_back( 1 );
			poly.indices.push_back( 2 );
			
			poly.wasSplitter = false;
			poly.plane = planeFromPoly(poly);

			poly.material = i;

			root.polys.push_back(poly);
		}
	}
}




bool CBspTree::polyExistsInNode(BspPoly* poly, BspNode* node, unsigned int polysToCheck)
{
	for (unsigned int i=0; i<polysToCheck; ++i)
	{
		if (node->polys[i] == *poly)
			return true;
	}
	return false;	
}




void CBspTree::createStats(const ox::scene::IMeshBuffer* array, unsigned int materialCount)
{
	stats.leafCounts = 0;
	stats.nodeCount = 0;
	stats.polySplits = 0;
	stats.totalIndexCount = 0;
	stats.totalVertexCount = 0;
	stats.originalTotalVertexCount = 0;
	stats.originalTotalIndexCount = 0;

	unsigned int i=0;

	for (i=0; i<materialCount; ++i)
	{
		stats.originalTotalVertexCount += array[i].getVertexCount();
		stats.originalTotalIndexCount += array[i].getIndexCount();
	}

	stats.materialCount = materialCount;

	stats.indexCount = new unsigned int[materialCount];
	for (i=0; i<materialCount; ++i)
		stats.indexCount[i] = 0;
	
	stats.vertexCount = new unsigned int[materialCount];
	for (i=0; i<materialCount; ++i)
		stats.vertexCount[i] = 0;

	stats.materialMap = new ox::video::SMaterial[materialCount];
}




void CBspTree::deleteStats()
{
	delete [] stats.indexCount;
	delete [] stats.vertexCount;
	delete [] stats.materialMap;
}

const SBspTreeStats* CBspTree::getStats()
{
	return &stats;
}

void CBspTree::createRenderBuffer()
{
	renderBuffer = new RenderBuffer[stats.materialCount];

	for (unsigned int i=0; i<stats.materialCount; ++i)
	{
		//renderBuffer[i].vertices.resize( = new unsigned short[stats.indexCount[i]];
		//memset(renderBuffer[i].iBuffer , 0, sizeof(unsigned short)*stats.indexCount[i]); // TODO wegtun
		//renderBuffer[i].vBuffer = new SStdVertex[stats.vertexCount[i]];
		//memset(renderBuffer[i].vBuffer , 0, sizeof(SStdVertex)*stats.vertexCount[i]); // TODO wegtun
		//renderBuffer[i].usedIndices = 0; // TODO: Wegtun
		//renderBuffer[i].usedVertices = 0;// TODO: wegtun
	}
}


void CBspTree::countPolyVertizes(const BspNode& node)
{
	for (unsigned int i=0; i<node.polys.size(); ++i)
	{
		stats.totalVertexCount += node.polys[i].vertices.size();
		stats.vertexCount[node.polys[i].material] += node.polys[i].vertices.size();
	}
}

void CBspTree::countPolyIndizes(const BspNode& node)
{
	for (unsigned int i=0; i<node.polys.size(); ++i)
	{
		stats.totalIndexCount += node.polys[i].vertices.size();
		stats.indexCount[node.polys[i].material] += node.polys[i].vertices.size();
	}
}

#ifdef _DEBUG
void CBspTree::debugOutputLeaf(const BspNode& node)
{
/*	char tmp[1024];
	sprintf(tmp, "(box: (%d, %d, %d), (%d, %d, %d)) countPolys:%d \n", (int)node.bbox.minEdge.x, (int)node.bbox.minEdge.y, (int)node.bbox.minEdge.z, (int)node.bbox.maxEdge.x, (int)node.bbox.maxEdge.y, (int)node.bbox.maxEdge.z, node.countPolys);
	logString(tmp);

	for (unsigned int i=0; i<node.countPolys; ++i)
	{
		logString("       polygon: ");

		for (unsigned int v=0; v<node.polys[i].countIndices; ++v)
		{
			SStdVertex* vtx = &node.polys[i].vertices[node.polys[i].indices[v]];
			sprintf(tmp, "(%d %d %d)", (int)vtx->x, (int)vtx->y, (int)vtx->z);
			logString(tmp);
		}
		sprintf(tmp, "  (plane nVect: %d %d %d)\n", (int)node.polys[i].plane.nVect.x, (int)node.polys[i].plane.nVect.y, (int)node.polys[i].plane.nVect.z);
		logString(tmp);
	}
*/
}

void CBspTree::debugValidateLeaf(const BspNode& node)
{
	/*
	if (node.isLeaf)
	{
		logString("begin NodeValidate\n");
		if (node.back)	logString("[CBspTree] Error validating leaf: back is nonzero.\n");
		if (node.front)	logString("[CBspTree] Error validating leaf: front is nonzero.\n");
		if (node.countPolys == 0) logString("[CBspTree] Error validating leaf: countPolys is zero.\n");

		for (unsigned int i=0; i<node.countPolys; ++i)
		{
			if (!node.polys[i].countVertices) logString("[CBspTree] Error validating leaf: no vertices in poly.\n");
			if (!node.polys[i].countIndices) logString("[CBspTree] Error validating leaf: no indices in poly.\n");

			if (node.polys[i].countIndices >= BSP_MAX_POLY_INDICES)
				logString("[CBspTree] Error validating leaf: too much indices in poly.\n");

			if (node.polys[i].countVertices >= BSP_MAX_POLY_VERTICES)
				logString("[CBspTree] Error validating leaf: too much vertices in poly.\n");
			
			for (unsigned int j=0; j<node.polys[i].countIndices; ++j)
				if (node.polys[i].indices[j] >= node.polys[i].countVertices)
					logString("[CBspTree] Error validating leaf: index pointing to invalid vertex.\n");

			if (node.polys[i].countIndices % 3 != 0)
				logString("[CBspTree] Error validating leaf: indexCount is not power of 3.\n");

			char tmp[255];
			sprintf(tmp, "Created poly%d got %d indices and %d vertices.\n", i, node.polys[i].countIndices, node.polys[i].countVertices);
			logString(tmp);
		}

		logString("end NodeValidate\n");
	}
	else
		logString("[CBspTree] Error validating leaf: not a leaf.\n");

	*/
}
#endif



void CBspTree::splitPoly(const BspPoly& poly, const ox::core::CPlane3d<float>& plane,
						 BspPoly& frontSplit, BspPoly& backSplit)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


const CBspTree::RenderBuffer* CBspTree::getRenderBuffer(unsigned int materialNr) const
{
	return &renderBuffer[materialNr];
}


} // end namespace
} // end namespace 

