// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFollowSpline.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorFollowSpline.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSceneNodeAnimatorFollowSpline::CSceneNodeAnimatorFollowSpline(unsigned int time,
	const ox::TArray< ox::core::CVector3d<float> >& points, float speed,
	float tightness)
: Points(points), Speed(speed), StartTime(time), Tightness(tightness)
{
	NumPoints = Points.size();
}



//! destructor
CSceneNodeAnimatorFollowSpline::~CSceneNodeAnimatorFollowSpline()
{
}



inline int CSceneNodeAnimatorFollowSpline::clamp(int idx, int size)
{
	return ( idx<0 ? size+idx : ( idx>=size ? idx-size : idx ) );
}


//! animates a scene node
void CSceneNodeAnimatorFollowSpline::animateNode(ISceneNode* node, unsigned int timeMs)
{
	ox::core::CVector3d<float> p, p0, p1, p2, p3;
	ox::core::CVector3d<float> t1, t2;

	float dt = ( (timeMs-StartTime) * Speed );
	int idx = static_cast< int >( 0.001f * dt ) % NumPoints;
	float u = 0.001f * fmodf( dt, 1000.0f );
    
	p0 = Points[ clamp( idx - 1, NumPoints ) ];
	p1 = Points[ clamp( idx, NumPoints ) ];
	p2 = Points[ clamp( idx + 1, NumPoints ) ];
	p3 = Points[ clamp( idx + 2, NumPoints ) ];

    // hermite polynomials
    float h1 = 2.0f * u * u * u - 3.0f * u * u + 1.0f;
    float h2 = -2.0f * u * u * u + 3.0f * u * u;
    float h3 = u * u * u - 2.0f * u * u + u;
    float h4 = u * u * u - u * u;

    // tangents
	t1 = ( p2 - p0 ) * Tightness;
	t2 = ( p3 - p1 ) * Tightness;

    // interpolated point
	p = p1 * h1 + p2 * h2 + t1 * h3 + t2 * h4;
		
	node->setPosition(p);
}


} // end namespace scene
} // end namespace daisy

