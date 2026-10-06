// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/matrix4.h, aabbox3d.h and plane3d.h (license:
// third_party/irrlicht-0.7/include/irrlicht.h) for the port's scene manager.
//
// The recovered ox::core::CMatrix4 and CAabbox3d declare only what the kept units use, so the
// Irrlicht members the scene graph needs live here as free functions. The arithmetic follows
// Irrlicht 0.7 operation for operation (float or double where it uses them), which is what the
// Mac 1.18 build's inlined copies show.

#ifndef PORT_SCENE_SCENEMATH_H
#define PORT_SCENE_SCENEMATH_H

#include <math.h>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"

namespace daisy {
namespace scene {

typedef ox::core::CVector3d<float> vector3df;
typedef ox::core::CAabbox3d<float> aabbox3df;
typedef ox::core::CMatrix4 matrix4;

//! M[12..14].
inline vector3df getTranslation(const matrix4& m)
{
    return vector3df(m.M[12], m.M[13], m.M[14]);
}

//! Sets the upper 3x3 to the rotation by the Euler angles (radians); the rest is unchanged.
inline void setRotationRadians(matrix4& m, const vector3df& rotation)
{
    double cr = cos(rotation.X);
    double sr = sin(rotation.X);
    double cp = cos(rotation.Y);
    double sp = sin(rotation.Y);
    double cy = cos(rotation.Z);
    double sy = sin(rotation.Z);

    m.M[0] = (float)(cp * cy);
    m.M[1] = (float)(cp * sy);
    m.M[2] = (float)(-sp);

    double srsp = sr * sp;
    double crsp = cr * sp;

    m.M[4] = (float)(srsp * cy - cr * sy);
    m.M[5] = (float)(srsp * sy + cr * cy);
    m.M[6] = (float)(sr * cp);

    m.M[8] = (float)(crsp * cy + sr * sy);
    m.M[9] = (float)(crsp * sy - sr * cy);
    m.M[10] = (float)(cr * cp);
}

//! Degrees to radians as Irrlicht does it: times (float)pi, then times 1/180 in float.
inline void setRotationDegrees(matrix4& m, const vector3df& rotation)
{
    setRotationRadians(m, rotation * 3.14159274f / 180.0f);
}

inline void setScale(matrix4& m, const vector3df& scale)
{
    m.M[0] = scale.X;
    m.M[5] = scale.Y;
    m.M[10] = scale.Z;
}

//! Transforms a point (w = 1): v' = v.x * row 0 + v.y * row 1 + v.z * row 2 + row 3.
inline void transformVect(const matrix4& m, vector3df& vect)
{
    float vector[3];
    vector[0] = vect.X * m.M[0] + vect.Y * m.M[4] + vect.Z * m.M[8] + m.M[12];
    vector[1] = vect.X * m.M[1] + vect.Y * m.M[5] + vect.Z * m.M[9] + m.M[13];
    vector[2] = vect.X * m.M[2] + vect.Y * m.M[6] + vect.Z * m.M[10] + m.M[14];
    vect.X = vector[0];
    vect.Y = vector[1];
    vect.Z = vector[2];
}

//! Multiplies the 4-vector v by the matrix in place (the same convention as transformVect).
inline void multiplyWith1x4Matrix(const matrix4& m, float* v)
{
    float mat[4] = { v[0], v[1], v[2], v[3] };
    v[0] = m.M[0] * mat[0] + m.M[4] * mat[1] + m.M[8] * mat[2] + m.M[12] * mat[3];
    v[1] = m.M[1] * mat[0] + m.M[5] * mat[1] + m.M[9] * mat[2] + m.M[13] * mat[3];
    v[2] = m.M[2] * mat[0] + m.M[6] * mat[1] + m.M[10] * mat[2] + m.M[14] * mat[3];
    v[3] = m.M[3] * mat[0] + m.M[7] * mat[1] + m.M[11] * mat[2] + m.M[15] * mat[3];
}

//! Irrlicht 0.7's left-handed perspective matrix, with its non-standard scale: with
//! h = cot(fov / 2) and w = h / aspect, M[0] = 2n / w and M[5] = 2n / h (see menu-scene.md).
inline void buildProjectionMatrixPerspectiveFovLH(matrix4& m, float fieldOfViewRadians, float aspectRatio,
    float zNear, float zFar)
{
    float h = (float)(cos(fieldOfViewRadians / 2) / sin(fieldOfViewRadians / 2));
    float w = h / aspectRatio;

    m(0, 0) = 2 * zNear / w;
    m(1, 0) = 0;
    m(2, 0) = 0;
    m(3, 0) = 0;

    m(0, 1) = 0;
    m(1, 1) = 2 * zNear / h;
    m(2, 1) = 0;
    m(3, 1) = 0;

    m(0, 2) = 0;
    m(1, 2) = 0;
    m(2, 2) = zFar / (zFar - zNear);
    m(3, 2) = 1;

    m(0, 3) = 0;
    m(1, 3) = 0;
    m(2, 3) = zNear * zFar / (zNear - zFar);
    m(3, 3) = 0;
}

inline void buildCameraLookAtMatrixLH(matrix4& m, const vector3df& position, const vector3df& target,
    const vector3df& upVector)
{
    vector3df zaxis = target - position;
    zaxis.normalize();

    vector3df xaxis = upVector.crossProduct(zaxis);
    xaxis.normalize();

    vector3df yaxis = zaxis.crossProduct(xaxis);

    m(0, 0) = xaxis.X;
    m(1, 0) = yaxis.X;
    m(2, 0) = zaxis.X;
    m(3, 0) = 0;

    m(0, 1) = xaxis.Y;
    m(1, 1) = yaxis.Y;
    m(2, 1) = zaxis.Y;
    m(3, 1) = 0;

    m(0, 2) = xaxis.Z;
    m(1, 2) = yaxis.Z;
    m(2, 2) = zaxis.Z;
    m(3, 2) = 0;

    m(0, 3) = -xaxis.dotProduct(position);
    m(1, 3) = -yaxis.dotProduct(position);
    m(2, 3) = -zaxis.dotProduct(position);
    m(3, 3) = 1.0f;
}

inline void resetBox(aabbox3df& box, const vector3df& point)
{
    box.MaxEdge = point;
    box.MinEdge = point;
}

inline void addInternalPoint(aabbox3df& box, const vector3df& p)
{
    if (p.X > box.MaxEdge.X) box.MaxEdge.X = p.X;
    if (p.Y > box.MaxEdge.Y) box.MaxEdge.Y = p.Y;
    if (p.Z > box.MaxEdge.Z) box.MaxEdge.Z = p.Z;

    if (p.X < box.MinEdge.X) box.MinEdge.X = p.X;
    if (p.Y < box.MinEdge.Y) box.MinEdge.Y = p.Y;
    if (p.Z < box.MinEdge.Z) box.MinEdge.Z = p.Z;
}

//! Swaps edge components so that MinEdge <= MaxEdge.
inline void repairBox(aabbox3df& box)
{
    float t;
    if (box.MinEdge.X > box.MaxEdge.X) { t = box.MinEdge.X; box.MinEdge.X = box.MaxEdge.X; box.MaxEdge.X = t; }
    if (box.MinEdge.Y > box.MaxEdge.Y) { t = box.MinEdge.Y; box.MinEdge.Y = box.MaxEdge.Y; box.MaxEdge.Y = t; }
    if (box.MinEdge.Z > box.MaxEdge.Z) { t = box.MinEdge.Z; box.MinEdge.Z = box.MaxEdge.Z; box.MaxEdge.Z = t; }
}

//! Transforms both edges and repairs the box (not the eight corners, as Irrlicht 0.7 does it).
inline void transformBox(const matrix4& m, aabbox3df& box)
{
    transformVect(m, box.MinEdge);
    transformVect(m, box.MaxEdge);
    repairBox(box);
}

inline bool intersectsWithBox(const aabbox3df& box, const aabbox3df& other)
{
    return box.MinEdge <= other.MaxEdge && box.MaxEdge >= other.MinEdge;
}

//! Separating-axis test of the box against a line segment given by its middle, unit direction
//! and half length.
inline bool intersectsWithLine(const aabbox3df& box, const vector3df& linemiddle, const vector3df& linevect,
    float halflength)
{
    const vector3df e = (box.MaxEdge - box.MinEdge) * 0.5f;
    const vector3df t = (box.MinEdge + e) - linemiddle;
    float r;

    if ((fabs(t.X) > e.X + halflength * fabs(linevect.X)) || (fabs(t.Y) > e.Y + halflength * fabs(linevect.Y)) ||
        (fabs(t.Z) > e.Z + halflength * fabs(linevect.Z)))
        return false;

    r = e.Y * (float)fabs(linevect.Z) + e.Z * (float)fabs(linevect.Y);
    if (fabs(t.Y * linevect.Z - t.Z * linevect.Y) > r)
        return false;

    r = e.X * (float)fabs(linevect.Z) + e.Z * (float)fabs(linevect.X);
    if (fabs(t.Z * linevect.X - t.X * linevect.Z) > r)
        return false;

    r = e.X * (float)fabs(linevect.Y) + e.Y * (float)fabs(linevect.X);
    if (fabs(t.X * linevect.Y - t.Y * linevect.X) > r)
        return false;

    return true;
}

//! Irrlicht 0.7's plane3d<f32>: D first, then the normal (the layout the Mac frustum uses).
struct SPlane3d
{
    SPlane3d()
        : D(0.0f), Normal(0, 1, 0) {}

    bool getIntersectionWithLine(const vector3df& linePoint, const vector3df& lineVect, vector3df& outIntersection) const
    {
        float t2 = Normal.dotProduct(lineVect);
        if (t2 == 0)
            return false;

        float t = -(Normal.dotProduct(linePoint) + D) / t2;
        outIntersection = linePoint + (lineVect * t);
        return true;
    }

    //! Irrlicht 0.7 takes the normals' lengths where the textbook formula has squared lengths; the
    //! frustum normals are unit length, so it makes no difference here.
    bool getIntersectionWithPlane(const SPlane3d& other, vector3df& outLinePoint, vector3df& outLineVect) const
    {
        double fn00 = Normal.getLength();
        double fn01 = Normal.dotProduct(other.Normal);
        double fn11 = other.Normal.getLength();
        double det = fn00 * fn11 - fn01 * fn01;

        if (fabs(det) < 1e-08f)
            return false;

        det = 1.0 / det;
        double fc0 = (fn11 * -D + fn01 * other.D) * det;
        double fc1 = (fn00 * -other.D + fn01 * D) * det;

        outLineVect = Normal.crossProduct(other.Normal);
        outLinePoint = Normal * (float)fc0 + other.Normal * (float)fc1;
        return true;
    }

    bool getIntersectionWithPlanes(const SPlane3d& o1, const SPlane3d& o2, vector3df& outPoint) const
    {
        vector3df linePoint, lineVect;
        if (getIntersectionWithPlane(o1, linePoint, lineVect))
            return o2.getIntersectionWithLine(linePoint, lineVect, outPoint);
        return false;
    }

    float D;
    vector3df Normal;
};

} // end namespace scene
} // end namespace daisy

#endif
