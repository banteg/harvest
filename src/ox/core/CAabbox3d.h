// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/aabbox3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only what
// recovered units use is declared.

#ifndef OX_CORE_CAABBOX3D_H
#define OX_CORE_CAABBOX3D_H

#include "CVector3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CLine3d.h"

namespace ox {
namespace core {

//! Axis aligned bounding box in 3d space.
template <class T>
class CAabbox3d
{
public:
    CAabbox3d<T> getInterpolated(const CAabbox3d<T>& other, float d) const
    {
        float inv = 1.0f - d;
        return CAabbox3d<T>((other.MinEdge*inv) + (MinEdge*d),
            (other.MaxEdge*inv) + (MaxEdge*d));
    }

    CVector3d<T> getExtent() const
    {
        return MaxEdge - MinEdge;
    }

    bool isEmpty() const
    {
        CVector3d<float> d = MinEdge - MaxEdge;
        if (d.X < 0) d.X = -d.X;
        if (d.Y < 0) d.Y = -d.Y;
        if (d.Z < 0) d.Z = -d.Z;

        return (d.X < ROUNDING_ERROR &&
                d.Y < ROUNDING_ERROR &&
                d.Z < ROUNDING_ERROR);
    }

    CVector3d<T> getCenter() const
    {
        return (MinEdge + MaxEdge) / 2;
    }

    bool intersectsWithLine(const CLine3d<T>& line) const
    {
        return intersectsWithLine(line.getMiddle(), line.getVector().normalize(),
                (T)(line.getLength() * 0.5));
    }

    bool intersectsWithLine(const CVector3d<T>& linemiddle,
                            const CVector3d<T>& linevect,
                            T halflength) const
    {
        const CVector3d<T> e = (MaxEdge - MinEdge) * (T)0.5;
        const CVector3d<T> t = (MinEdge + e) - linemiddle;
        float r;

        if ((fabs(t.X) > e.X + halflength * fabs(linevect.X)) ||
            (fabs(t.Y) > e.Y + halflength * fabs(linevect.Y)) ||
            (fabs(t.Z) > e.Z + halflength * fabs(linevect.Z)) )
            return false;

        r = e.Y * (T)fabs(linevect.Z) + e.Z * (T)fabs(linevect.Y);
        if (fabs(t.Y*linevect.Z - t.Z*linevect.Y) > r )
            return false;

        r = e.X * (T)fabs(linevect.Z) + e.Z * (T)fabs(linevect.X);
        if (fabs(t.Z*linevect.X - t.X*linevect.Z) > r )
            return false;

        r = e.X * (T)fabs(linevect.Y) + e.Y * (T)fabs(linevect.X);
        if (fabs(t.X*linevect.Y - t.Y*linevect.X) > r)
            return false;

        return true;
    }

    EIntersectionRelation3D classifyPlaneRelation(const CPlane3d<float>& plane) const
    {
        CVector3d<T> nearPoint(MaxEdge);
        CVector3d<T> farPoint(MinEdge);

        if (plane.Normal.X > (T)0)
        {
            nearPoint.X = MinEdge.X;
            farPoint.X = MaxEdge.X;
        }

        if (plane.Normal.Y > (T)0)
        {
            nearPoint.Y = MinEdge.Y;
            farPoint.Y = MaxEdge.Y;
        }

        if (plane.Normal.Z > (T)0)
        {
            nearPoint.Z = MinEdge.Z;
            farPoint.Z = MaxEdge.Z;
        }

        if (plane.Normal.dotProduct(nearPoint) + plane.D > (T)0)
            return ISREL3D_FRONT;

        if (plane.Normal.dotProduct(farPoint) + plane.D > (T)0)
            return ISREL3D_CLIPPED;

        return ISREL3D_BACK;
    }

    CAabbox3d(const CVector3d<T>& min, const CVector3d<T>& max): MinEdge(min), MaxEdge(max) {}

    CAabbox3d(const CVector3d<T>& init): MinEdge(init), MaxEdge(init) {}

    CAabbox3d(T minx, T miny, T minz, T maxx, T maxy, T maxz): MinEdge(minx, miny, minz), MaxEdge(maxx, maxy, maxz) {}

    bool intersectsWithBox(const CAabbox3d<T>& other) const
    {
        return (MinEdge <= other.MaxEdge && MaxEdge >= other.MinEdge);
    }

    bool isPointTotalInside(const CVector3d<T>& p) const
    {
        return (    p.X > MinEdge.X && p.X < MaxEdge.X &&
                    p.Y > MinEdge.Y && p.Y < MaxEdge.Y &&
                    p.Z > MinEdge.Z && p.Z < MaxEdge.Z);
    }

    bool isPointInside(const CVector3d<T>& p) const
    {
        return (    p.X >= MinEdge.X && p.X <= MaxEdge.X &&
                    p.Y >= MinEdge.Y && p.Y <= MaxEdge.Y &&
                    p.Z >= MinEdge.Z && p.Z <= MaxEdge.Z);
    }

    void reset(T x, T y, T z)
    {
        MaxEdge.set(x,y,z);
        MinEdge = MaxEdge;
    }

    void reset(const CAabbox3d<T>& initValue)
    {
        *this = initValue;
    }

    void reset(const CVector3d<T>& initValue)
    {
        MaxEdge = initValue;
        MinEdge = initValue;
    }

    void addInternalBox(const CAabbox3d<T>& b)
    {
        addInternalPoint(b.MaxEdge);
        addInternalPoint(b.MinEdge);
    }

    void addInternalPoint(const CVector3d<T>& p)
    {
        addInternalPoint(p.X, p.Y, p.Z);
    }

    void addInternalPoint(T x, T y, T z)
    {
        if (x>MaxEdge.X) MaxEdge.X = x;
        if (y>MaxEdge.Y) MaxEdge.Y = y;
        if (z>MaxEdge.Z) MaxEdge.Z = z;

        if (x<MinEdge.X) MinEdge.X = x;
        if (y<MinEdge.Y) MinEdge.Y = y;
        if (z<MinEdge.Z) MinEdge.Z = z;
    }

    inline bool operator!=(const CAabbox3d<T>& other) const { return !(MinEdge == other.MinEdge && other.MaxEdge == MaxEdge);}

    inline bool operator==(const CAabbox3d<T>& other) const { return (MinEdge == other.MinEdge && other.MaxEdge == MaxEdge);}

    void repair()
    {
        T t;

        if (MinEdge.X > MaxEdge.X)
            { t=MinEdge.X; MinEdge.X = MaxEdge.X; MaxEdge.X=t; }
        if (MinEdge.Y > MaxEdge.Y)
            { t=MinEdge.Y; MinEdge.Y = MaxEdge.Y; MaxEdge.Y=t; }
        if (MinEdge.Z > MaxEdge.Z)
            { t=MinEdge.Z; MinEdge.Z = MaxEdge.Z; MaxEdge.Z=t; }
    }

    CAabbox3d() : MinEdge(-1, -1, -1), MaxEdge(1, 1, 1) {}

    //! Writes the eight corners of the box to edges.
    void getEdges(CVector3d<T>* edges) const
    {
        CVector3d<T> middle = (MinEdge + MaxEdge) / 2;
        CVector3d<T> diag = middle - MaxEdge;

        edges[0].set(middle.X + diag.X, middle.Y + diag.Y, middle.Z + diag.Z);
        edges[1].set(middle.X + diag.X, middle.Y - diag.Y, middle.Z + diag.Z);
        edges[2].set(middle.X + diag.X, middle.Y + diag.Y, middle.Z - diag.Z);
        edges[3].set(middle.X + diag.X, middle.Y - diag.Y, middle.Z - diag.Z);
        edges[4].set(middle.X - diag.X, middle.Y + diag.Y, middle.Z + diag.Z);
        edges[5].set(middle.X - diag.X, middle.Y - diag.Y, middle.Z + diag.Z);
        edges[6].set(middle.X - diag.X, middle.Y + diag.Y, middle.Z - diag.Z);
        edges[7].set(middle.X - diag.X, middle.Y - diag.Y, middle.Z - diag.Z);
    }

    CVector3d<T> MinEdge;
    CVector3d<T> MaxEdge;
};

} // end namespace core
} // end namespace ox

#endif
