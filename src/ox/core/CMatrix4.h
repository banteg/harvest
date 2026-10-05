// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/matrix4.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only construction
// as the identity matrix is recovered.

#ifndef OX_CORE_CMATRIX4_H
#define OX_CORE_CMATRIX4_H

namespace ox {
namespace core {

//! 4x4 matrix, stored row by row.
class CMatrix4
{
public:
    //! Constructs an identity matrix.
    CMatrix4()
    {
        makeIdentity();
    }

    void makeIdentity()
    {
        for (int i = 0; i < 16; ++i)
            M[i] = 0.0f;
        M[0] = M[5] = M[10] = M[15] = 1.0f;
    }

    float M[16];
};

} // end namespace core
} // end namespace ox

#endif
