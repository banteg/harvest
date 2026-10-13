// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/matrix4.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only the members
// recovered units use are declared.

#ifndef OX_CORE_CMATRIX4_H
#define OX_CORE_CMATRIX4_H

#include "CVector3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CAabbox3d.h"

namespace ox {
namespace core {

//! 4x4 matrix, row major with the translation in the 4th row (D3D style).
class CMatrix4
{
public:
    void multiplyWith1x4Matrix(float* matrix) const;
    void transformPlane( CPlane3d<float> &plane) const;
    void transformPlane( const CPlane3d<float> &in, CPlane3d<float> &out) const;
    void inverseTranslateVect( CVector3d<float>& vect ) const;
    void inverseRotateVect( CVector3d<float>& vect ) const;
    CVector3d<float> getTranslation() const;
    void transformBox( CAabbox3d<float> &box) const;
    void setRotationDegrees( const CVector3d<float>& rotation );
    void setRotationRadians( const CVector3d<float>& rotation );
    void setScale( const CVector3d<float>& scale );
    void transformVect( CVector3d<float>& vect) const;
    void transformVect( const CVector3d<float>& in, CVector3d<float>& out) const;
    CMatrix4();

    float& operator()(int row, int col) { return M[col * 4 + row]; }
    const float& operator()(int row, int col) const { return M[col * 4 + row]; }

    CMatrix4& operator=(const CMatrix4& other);
    CMatrix4& operator*=(const CMatrix4& other);
    CMatrix4 operator*(const CMatrix4& other) const;

    void makeIdentity();
    void setTranslation(const CVector3d<float>& translation);

    //! Writes the inverse of this matrix to out; returns false if there is none.
    bool getInverse(CMatrix4& out);
    CMatrix4 getTransposed();

    float M[16];
};

inline CMatrix4::CMatrix4()
{
    makeIdentity();
}

inline CMatrix4& CMatrix4::operator=(const CMatrix4& other)
{
    for (int i = 0; i < 16; ++i)
        M[i] = other.M[i];
    return *this;
}

inline CMatrix4& CMatrix4::operator*=(const CMatrix4& other)
{
    float newMatrix[16];
    const float* m1 = M;
    const float* m2 = other.M;

    newMatrix[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
    newMatrix[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
    newMatrix[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
    newMatrix[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];

    newMatrix[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
    newMatrix[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
    newMatrix[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
    newMatrix[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];

    newMatrix[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
    newMatrix[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
    newMatrix[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
    newMatrix[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];

    newMatrix[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
    newMatrix[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
    newMatrix[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
    newMatrix[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];

    for (int i = 0; i < 16; ++i)
        M[i] = newMatrix[i];

    return *this;
}

inline CMatrix4 CMatrix4::operator*(const CMatrix4& other) const
{
    CMatrix4 tmtrx;
    const float* m1 = M;
    const float* m2 = other.M;
    float* m3 = tmtrx.M;

    m3[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
    m3[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
    m3[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
    m3[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];

    m3[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
    m3[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
    m3[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
    m3[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];

    m3[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
    m3[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
    m3[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
    m3[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];

    m3[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
    m3[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
    m3[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
    m3[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];

    return tmtrx;
}

inline void CMatrix4::makeIdentity()
{
    for (int i = 0; i < 16; ++i)
        M[i] = 0.0f;
    M[0] = M[5] = M[10] = M[15] = 1;
}

inline void CMatrix4::setTranslation(const CVector3d<float>& translation)
{
    M[12] = translation.X;
    M[13] = translation.Y;
    M[14] = translation.Z;
}

inline bool CMatrix4::getInverse(CMatrix4& out)
{
    // Cramer's rule.
    const CMatrix4& m = *this;

    float d = (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3)) -
        (m(0, 0) * m(2, 1) - m(2, 0) * m(0, 1)) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3)) +
        (m(0, 0) * m(3, 1) - m(3, 0) * m(0, 1)) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3)) +
        (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3)) -
        (m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1)) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3)) +
        (m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1)) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3));

    if (d == 0.f)
        return false;

    d = 1.f / d;

    out(0, 0) = d * (m(1, 1) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3)) + m(2, 1) * (m(3, 2) * m(1, 3) - m(1, 2) * m(3, 3)) + m(3, 1) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3)));
    out(1, 0) = d * (m(1, 2) * (m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3)) + m(2, 2) * (m(3, 0) * m(1, 3) - m(1, 0) * m(3, 3)) + m(3, 2) * (m(1, 0) * m(2, 3) - m(2, 0) * m(1, 3)));
    out(2, 0) = d * (m(1, 3) * (m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1)) + m(2, 3) * (m(3, 0) * m(1, 1) - m(1, 0) * m(3, 1)) + m(3, 3) * (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)));
    out(3, 0) = d * (m(1, 0) * (m(3, 1) * m(2, 2) - m(2, 1) * m(3, 2)) + m(2, 0) * (m(1, 1) * m(3, 2) - m(3, 1) * m(1, 2)) + m(3, 0) * (m(2, 1) * m(1, 2) - m(1, 1) * m(2, 2)));
    out(0, 1) = d * (m(2, 1) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3)) + m(3, 1) * (m(2, 2) * m(0, 3) - m(0, 2) * m(2, 3)) + m(0, 1) * (m(3, 2) * m(2, 3) - m(2, 2) * m(3, 3)));
    out(1, 1) = d * (m(2, 2) * (m(0, 0) * m(3, 3) - m(3, 0) * m(0, 3)) + m(3, 2) * (m(2, 0) * m(0, 3) - m(0, 0) * m(2, 3)) + m(0, 2) * (m(3, 0) * m(2, 3) - m(2, 0) * m(3, 3)));
    out(2, 1) = d * (m(2, 3) * (m(0, 0) * m(3, 1) - m(3, 0) * m(0, 1)) + m(3, 3) * (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) + m(0, 3) * (m(3, 0) * m(2, 1) - m(2, 0) * m(3, 1)));
    out(3, 1) = d * (m(2, 0) * (m(3, 1) * m(0, 2) - m(0, 1) * m(3, 2)) + m(3, 0) * (m(0, 1) * m(2, 2) - m(2, 1) * m(0, 2)) + m(0, 0) * (m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2)));
    out(0, 2) = d * (m(3, 1) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3)) + m(0, 1) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3)) + m(1, 1) * (m(3, 2) * m(0, 3) - m(0, 2) * m(3, 3)));
    out(1, 2) = d * (m(3, 2) * (m(0, 0) * m(1, 3) - m(1, 0) * m(0, 3)) + m(0, 2) * (m(1, 0) * m(3, 3) - m(3, 0) * m(1, 3)) + m(1, 2) * (m(3, 0) * m(0, 3) - m(0, 0) * m(3, 3)));
    out(2, 2) = d * (m(3, 3) * (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) + m(0, 3) * (m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1)) + m(1, 3) * (m(3, 0) * m(0, 1) - m(0, 0) * m(3, 1)));
    out(3, 2) = d * (m(3, 0) * (m(1, 1) * m(0, 2) - m(0, 1) * m(1, 2)) + m(0, 0) * (m(3, 1) * m(1, 2) - m(1, 1) * m(3, 2)) + m(1, 0) * (m(0, 1) * m(3, 2) - m(3, 1) * m(0, 2)));
    out(0, 3) = d * (m(0, 1) * (m(2, 2) * m(1, 3) - m(1, 2) * m(2, 3)) + m(1, 1) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3)) + m(2, 1) * (m(1, 2) * m(0, 3) - m(0, 2) * m(1, 3)));
    out(1, 3) = d * (m(0, 2) * (m(2, 0) * m(1, 3) - m(1, 0) * m(2, 3)) + m(1, 2) * (m(0, 0) * m(2, 3) - m(2, 0) * m(0, 3)) + m(2, 2) * (m(1, 0) * m(0, 3) - m(0, 0) * m(1, 3)));
    out(2, 3) = d * (m(0, 3) * (m(2, 0) * m(1, 1) - m(1, 0) * m(2, 1)) + m(1, 3) * (m(0, 0) * m(2, 1) - m(2, 0) * m(0, 1)) + m(2, 3) * (m(1, 0) * m(0, 1) - m(0, 0) * m(1, 1)));
    out(3, 3) = d * (m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) + m(1, 0) * (m(2, 1) * m(0, 2) - m(0, 1) * m(2, 2)) + m(2, 0) * (m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2)));

    return true;
}

inline CMatrix4 CMatrix4::getTransposed()
{
    CMatrix4 t;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            t(r, c) = (*this)(c, r);
    return t;
}

inline void CMatrix4::transformVect( CVector3d<float>& vect) const
{
    float vector[3];

    vector[0] = vect.X*M[0] + vect.Y*M[4] + vect.Z*M[8] + M[12];
    vector[1] = vect.X*M[1] + vect.Y*M[5] + vect.Z*M[9] + M[13];
    vector[2] = vect.X*M[2] + vect.Y*M[6] + vect.Z*M[10] + M[14];

    vect.X = vector[0];
    vect.Y = vector[1];
    vect.Z = vector[2];
}

inline void CMatrix4::transformVect( const CVector3d<float>& in, CVector3d<float>& out) const
{
    out.X = in.X*M[0] + in.Y*M[4] + in.Z*M[8] + M[12];
    out.Y = in.X*M[1] + in.Y*M[5] + in.Z*M[9] + M[13];
    out.Z = in.X*M[2] + in.Y*M[6] + in.Z*M[10] + M[14];
}

inline void CMatrix4::setScale( const CVector3d<float>& scale )
{
    M[0] = scale.X;
    M[5] = scale.Y;
    M[10] = scale.Z;
}

inline void CMatrix4::setRotationRadians( const CVector3d<float>& rotation )
{
    double cr = cos( rotation.X );
    double sr = sin( rotation.X );
    double cp = cos( rotation.Y );
    double sp = sin( rotation.Y );
    double cy = cos( rotation.Z );
    double sy = sin( rotation.Z );

    M[0] = (float)( cp*cy );
    M[1] = (float)( cp*sy );
    M[2] = (float)( -sp );

    double srsp = sr*sp;
    double crsp = cr*sp;

    M[4] = (float)( srsp*cy-cr*sy );
    M[5] = (float)( srsp*sy+cr*cy );
    M[6] = (float)( sr*cp );

    M[8] = (float)( crsp*cy+sr*sy );
    M[9] = (float)( crsp*sy-sr*cy );
    M[10] = (float)( cr*cp );
}

inline void CMatrix4::setRotationDegrees( const CVector3d<float>& rotation )
{
    setRotationRadians( rotation * (float)3.1415926535897932384626433832795 / 180.0 );
}

inline void CMatrix4::transformBox( CAabbox3d<float> &box) const
{
    transformVect(box.MinEdge);
    transformVect(box.MaxEdge);
    box.repair();
}

inline CVector3d<float> CMatrix4::getTranslation() const
{
    return CVector3d<float>(M[12], M[13], M[14]);
}

inline void CMatrix4::inverseRotateVect( CVector3d<float>& vect ) const
{
    CVector3d<float> tmp = vect;
    vect.X = tmp.X*M[0] + tmp.Y*M[1] + tmp.Z*M[2];
    vect.Y = tmp.X*M[4] + tmp.Y*M[5] + tmp.Z*M[6];
    vect.Z = tmp.X*M[8] + tmp.Y*M[9] + tmp.Z*M[10];
}

inline void CMatrix4::inverseTranslateVect( CVector3d<float>& vect ) const
{
    vect.X = vect.X-M[12];
    vect.Y = vect.Y-M[13];
    vect.Z = vect.Z-M[14];
}

inline void CMatrix4::transformPlane( CPlane3d<float> &plane) const
{
    CVector3d<float> member;
    transformVect(plane.getMemberPoint(), member);

    CVector3d<float> origin(0,0,0);
    transformVect(plane.Normal);
    transformVect(origin);

    plane.Normal -= origin;
    plane.D = - member.dotProduct(plane.Normal);
}

inline void CMatrix4::transformPlane( const CPlane3d<float> &in, CPlane3d<float> &out) const
{
    out = in;
    transformPlane( out );
}

inline void CMatrix4::multiplyWith1x4Matrix(float* matrix) const
{
    /*
    0  1  2  3
    4  5  6  7
    8  9  10 11
    12 13 14 15
    */

    float mat[4];
    mat[0] = matrix[0];
    mat[1] = matrix[1];
    mat[2] = matrix[2];
    mat[3] = matrix[3];

    matrix[0] = M[0]*mat[0] + M[4]*mat[1] + M[8]*mat[2] + M[12]*mat[3];
    matrix[1] = M[1]*mat[0] + M[5]*mat[1] + M[9]*mat[2] + M[13]*mat[3];
    matrix[2] = M[2]*mat[0] + M[6]*mat[1] + M[10]*mat[2] + M[14]*mat[3];
    matrix[3] = M[3]*mat[0] + M[7]*mat[1] + M[11]*mat[2] + M[15]*mat[3];
}

} // end namespace core
} // end namespace ox

#endif
