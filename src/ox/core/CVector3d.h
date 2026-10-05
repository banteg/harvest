// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/vector3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: the rotations and
// interpolation are not recovered yet.

#ifndef OX_CORE_CVECTOR3D_H
#define OX_CORE_CVECTOR3D_H

#include <math.h>

namespace ox {
namespace core {

//! 3d vector template class with lots of operators and methods.
template <class T>
class CVector3d
{
public:
    CVector3d() : X(0), Y(0), Z(0) {}
    CVector3d(T nx, T ny, T nz) : X(nx), Y(ny), Z(nz) {}
    CVector3d(const CVector3d<T>& other) : X(other.X), Y(other.Y), Z(other.Z) {}

    // operators

    CVector3d<T> operator-() const { return CVector3d<T>(-X, -Y, -Z); }

    CVector3d<T>& operator=(const CVector3d<T>& other) { X = other.X; Y = other.Y; Z = other.Z; return *this; }

    CVector3d<T> operator+(const CVector3d<T>& other) const { return CVector3d<T>(X + other.X, Y + other.Y, Z + other.Z); }
    CVector3d<T>& operator+=(const CVector3d<T>& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }

    CVector3d<T> operator-(const CVector3d<T>& other) const { return CVector3d<T>(X - other.X, Y - other.Y, Z - other.Z); }
    CVector3d<T>& operator-=(const CVector3d<T>& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }

    CVector3d<T> operator*(const CVector3d<T>& other) const { return CVector3d<T>(X * other.X, Y * other.Y, Z * other.Z); }
    CVector3d<T>& operator*=(const CVector3d<T>& other) { X *= other.X; Y *= other.Y; Z *= other.Z; return *this; }
    CVector3d<T> operator*(const T v) const { return CVector3d<T>(X * v, Y * v, Z * v); }
    CVector3d<T>& operator*=(const T v) { X *= v; Y *= v; Z *= v; return *this; }

    CVector3d<T> operator/(const CVector3d<T>& other) const { return CVector3d<T>(X / other.X, Y / other.Y, Z / other.Z); }
    CVector3d<T>& operator/=(const CVector3d<T>& other) { X /= other.X; Y /= other.Y; Z /= other.Z; return *this; }
    CVector3d<T> operator/(const T v) const { T i = (T)1.0 / v; return CVector3d<T>(X * i, Y * i, Z * i); }
    CVector3d<T>& operator/=(const T v) { T i = (T)1.0 / v; X *= i; Y *= i; Z *= i; return *this; }

    bool operator<=(const CVector3d<T>& other) const { return X <= other.X && Y <= other.Y && Z <= other.Z; }
    bool operator>=(const CVector3d<T>& other) const { return X >= other.X && Y >= other.Y && Z >= other.Z; }

    bool operator==(const CVector3d<T>& other) const { return other.X == X && other.Y == Y && other.Z == Z; }
    bool operator!=(const CVector3d<T>& other) const { return other.X != X || other.Y != Y || other.Z != Z; }

    // functions

    void set(const T nx, const T ny, const T nz) { X = nx; Y = ny; Z = nz; }
    void set(const CVector3d<T>& p) { X = p.X; Y = p.Y; Z = p.Z; }

    //! Returns length of the vector.
    double getLength() const { return sqrt(X * X + Y * Y + Z * Z); }

    //! Returns squared length of the vector.
    double getLengthSQ() const { return X * X + Y * Y + Z * Z; }

    //! Returns the dot product with another vector.
    T dotProduct(const CVector3d<T>& other) const
    {
        return X * other.X + Y * other.Y + Z * other.Z;
    }

    //! Returns distance from another point.
    double getDistanceFrom(const CVector3d<T>& other) const
    {
        double vx = X - other.X; double vy = Y - other.Y; double vz = Z - other.Z;
        return sqrt(vx * vx + vy * vy + vz * vz);
    }

    //! Returns squared distance from another point.
    float getDistanceFromSQ(const CVector3d<T>& other) const
    {
        float vx = X - other.X; float vy = Y - other.Y; float vz = Z - other.Z;
        return (vx * vx + vy * vy + vz * vz);
    }

    //! Normalizes the vector.
    CVector3d<T>& normalize()
    {
        T l = (T)getLength();
        if (l == 0)
            return *this;

        l = (T)1.0 / l;
        X *= l;
        Y *= l;
        Z *= l;
        return *this;
    }

    CVector3d<T> crossProduct(const CVector3d<T>& p) const
    {
        return CVector3d<T>(Y * p.Z - Z * p.Y, Z * p.X - X * p.Z, X * p.Y - Y * p.X);
    }

    // member variables

    T X, Y, Z;
};

} // end namespace core
} // end namespace ox

#endif
