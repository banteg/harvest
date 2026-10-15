// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/quaternion.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source.

#ifndef __IRR_QUATERNION_H_INCLUDED__
#define __IRR_QUATERNION_H_INCLUDED__

#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/core/irrMath.h"

namespace ox
{
namespace core
{

//! Quaternion class.
class CQuaternion
{
	public:

		//! Default Constructor
		CQuaternion();

		//! Constructor
		CQuaternion(float X, float Y, float Z, float W);

		//! Constructor which converts euler angles to a CQuaternion
		CQuaternion(float x, float y, float z);

		//! Constructor which converts a matrix to a CQuaternion
		CQuaternion(const CMatrix4& mat);

		//! equal operator
		bool operator==(const CQuaternion& other) const;

		//! assignment operator
		CQuaternion& operator=(const CQuaternion& other);

		//! matrix assignment operator
		CQuaternion& operator=(const CMatrix4& other);

		//! add operator
		CQuaternion operator+(const CQuaternion& other) const;

		//! multiplication operator
		CQuaternion operator*(const CQuaternion& other) const;

		//! multiplication operator
		CQuaternion operator*(float s) const;

		//! multiplication operator
		CQuaternion& operator*=(float s);

		//! multiplication operator
		CVector3d<float> operator* (const CVector3d<float>& v) const;

		//! multiplication operator
		CQuaternion& operator*=(const CQuaternion& other);

		//! calculates the dot product
		float getDotProduct(const CQuaternion& other) const;

		//! sets new CQuaternion
		void set(float x, float y, float z, float w);

		//! sets new CQuaternion based on euler angles
		void set(float x, float y, float z);

		//! normalizes the CQuaternion
		CQuaternion& normalize();

		//! Creates a matrix from this CQuaternion
		CMatrix4 getMatrix() const;

		//! Inverts this CQuaternion
        void makeInverse();

		//! Interpolates the CQuaternion between to quaternions based on time
		CQuaternion slerp(CQuaternion q1, CQuaternion q2, float time);

		//! axis must be unit length
		//! The CQuaternion representing the rotation is
		//!  q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)
		void fromAngleAxis (float angle, const CVector3d<float>& axis);

		void toEuler(CVector3d<float>& euler);

		float X, Y, Z, W;
};


//! Default Constructor
inline CQuaternion::CQuaternion()
: X(0.0f), Y(0.0f), Z(0.0f), W(1.0f)
{
}

//! Constructor
inline CQuaternion::CQuaternion(float x, float y, float z, float w)
: X(x), Y(y), Z(z), W(w)
{
}

//! Constructor which converts euler angles to a CQuaternion
inline CQuaternion::CQuaternion(float x, float y, float z)
{
	set(x,y,z);
}


//! Constructor which converts a matrix to a CQuaternion
inline CQuaternion::CQuaternion(const CMatrix4& mat)
{
	(*this) = mat;
}


//! equal operator
inline bool CQuaternion::operator==(const CQuaternion& other) const
{
	if(X != other.X)
		return false;
	if(Y != other.Y)
		return false;
	if(Z != other.Z)
		return false;
	if(W != other.W)
		return false;

	return true;
}


//! assignment operator
inline CQuaternion& CQuaternion::operator=(const CQuaternion& other)
{
	X = other.X;
	Y = other.Y;
	Z = other.Z;
	W = other.W;
	return *this;
}


//! matrix assignment operator
inline CQuaternion& CQuaternion::operator=(const CMatrix4& m)
{
	float diag = m(0,0) + m(1,1) + m(2,2) + 1;
	float scale = 0.0f;

	if( diag > 0.0f )
	{
		scale = sqrtf(diag) * 2.0f; // get scale from diagonal

		// TODO: speed this up
		X = ( m(1,2) - m(2,1)) / scale;
		Y = ( m(2,0) - m(0,2)) / scale;
		Z = ( m(0,1) - m(1,0)) / scale;
		W = 0.25f * scale;
	}
	else
	{
		if ( m(0,0) > m(1,1) && m(0,0) > m(2,2))
		{
			// 1st element of diag is greatest value
			// find scale according to 1st element, and double it
			scale = sqrtf( 1.0f + m(0,0) - m(1,1) - m(2,2)) * 2.0f;

			// TODO: speed this up
			X = 0.25f * scale;
			Y = (m(0,1) + m(1,0)) / scale;
			Z = (m(2,0) + m(0,2)) / scale;
			W = (m(1,2) - m(2,1)) / scale;
		}
		else if ( m(1,1) > m(2,2))
		{
			// 2nd element of diag is greatest value
			// find scale according to 2nd element, and double it
			scale = sqrtf( 1.0f + m(1,1) - m(0,0) - m(2,2)) * 2.0f;

			// TODO: speed this up
			X = (m(0,1) + m(1,0) ) / scale;
			Y = 0.25f * scale;
			Z = (m(1,2) + m(2,1) ) / scale;
			W = (m(2,0) - m(0,2) ) / scale;
		}
		else
		{
			// 3rd element of diag is greatest value
			// find scale according to 3rd element, and double it
			scale  = sqrtf( 1.0f + m(2,2) - m(0,0) - m(1,1)) * 2.0f;

			// TODO: speed this up
			X = (m(2,0) + m(0,2)) / scale;
			Y = (m(1,2) + m(2,1)) / scale;
			Z = 0.25f * scale;
			W = (m(0,1) - m(1,0)) / scale;
		}
	}

	normalize();
	return *this;
}


//! multiplication operator
inline CQuaternion CQuaternion::operator*(const CQuaternion& other) const
{
	CQuaternion tmp;

	tmp.W = (other.W * W) - (other.X * X) - (other.Y * Y) - (other.Z * Z);
	tmp.X = (other.W * X) + (other.X * W) + (other.Y * Z) - (other.Z * Y);
	tmp.Y = (other.W * Y) + (other.Y * W) + (other.Z * X) - (other.X * Z);
	tmp.Z = (other.W * Z) + (other.Z * W) + (other.X * Y) - (other.Y * X);

	return tmp;
}


//! multiplication operator
inline CQuaternion CQuaternion::operator*(float s) const
{
	return CQuaternion(s*X, s*Y, s*Z, s*W);
}

//! multiplication operator
inline CQuaternion& CQuaternion::operator*=(float s)
{
	X *= s; Y*=s; Z*=s; W*=s;
	return *this;
}

//! multiplication operator
inline CQuaternion& CQuaternion::operator*=(const CQuaternion& other)
{
	*this = other * (*this);
	return *this;
}

//! add operator
inline CQuaternion CQuaternion::operator+(const CQuaternion& b) const
{
	return CQuaternion(X+b.X, Y+b.Y, Z+b.Z, W+b.W);
}


//! Creates a matrix from this CQuaternion
inline CMatrix4 CQuaternion::getMatrix() const
{
	CMatrix4 m;

	m(0,0) = 1.0f - 2.0f*Y*Y - 2.0f*Z*Z;
	m(0,1) = 2.0f*X*Y + 2.0f*Z*W;
	m(0,2) = 2.0f*X*Z - 2.0f*Y*W;
	m(0,3) = 0.0f;

	m(1,0) = 2.0f*X*Y - 2.0f*Z*W;
	m(1,1) = 1.0f - 2.0f*X*X - 2.0f*Z*Z;
	m(1,2) = 2.0f*Z*Y + 2.0f*X*W;
	m(1,3) = 0.0f;

	m(2,0) = 2.0f*X*Z + 2.0f*Y*W;
	m(2,1) = 2.0f*Z*Y - 2.0f*X*W;
	m(2,2) = 1.0f - 2.0f*X*X - 2.0f*Y*Y;
	m(2,3) = 0.0f;

	m(3,0) = 0.0f;
	m(3,1) = 0.0f;
	m(3,2) = 0.0f;
	m(3,3) = 1.0f;

	return m;
}


//! Inverts this CQuaternion
inline void CQuaternion::makeInverse()
{
	X = -X; Y = -Y; Z = -Z;
}

//! sets new CQuaternion
inline void CQuaternion::set(float x, float y, float z, float w)
{
	X = x;
	Y = y;
	Z = z;
	W = w;
}

//! sets new CQuaternion based on euler angles
inline void CQuaternion::set(float x, float y, float z)
{
	float angle;

	angle = x * 0.5f;
	float sr = (float)sin(angle);
	float cr = (float)cos(angle);

	angle = y * 0.5f;
    float sp = (float)sin(angle);
	float cp = (float)cos(angle);

	angle = z * 0.5f;
	float sy = (float)sin(angle);
	float cy = (float)cos(angle);

	float cpcy = cp * cy;
	float spcy = sp * cy;
	float cpsy = cp * sy;
	float spsy = sp * sy;

	X = sr * cpcy - cr * spsy;
	Y = cr * spcy + sr * cpsy;
	Z = cr * cpsy - sr * spcy;
	W = cr * cpcy + sr * spsy;

	normalize();
}

//! normalizes the CQuaternion
inline CQuaternion& CQuaternion::normalize()
{
	float n = X*X + Y*Y + Z*Z + W*W;

	if (n == 1)
		return *this;

	n = 1.0f / sqrtf(n);
	X *= n;
	Y *= n;
	Z *= n;
	W *= n;

	return *this;
}


// Interpolates the CQuaternion between to quaternions based on time
inline CQuaternion CQuaternion::slerp(CQuaternion q1, CQuaternion q2, float time)
{
    float angle = q1.getDotProduct(q2);

    if (angle < 0.0f)
    {
		q1 *= -1.0f;
		angle *= -1.0f;
    }

	float scale;
	float invscale;

    if ((angle + 1.0f) > 0.05f)
    {
		if ((1.0f - angle) >= 0.05f)  // spherical interpolation
		{
			float theta = (float)acos(angle);
			float invsintheta = 1.0f / (float)sin(theta);
			scale = (float)sin(theta * (1.0f-time)) * invsintheta;
			invscale = (float)sin(theta * time) * invsintheta;
		}
		else // linear interploation
		{
			scale = 1.0f - time;
			invscale = time;
		}
    }
    else
    {
        q2 = CQuaternion(-q1.Y, q1.X, -q1.W, q1.Z);
        scale = (float)sin(PI * (0.5f - time));
        invscale = (float)sin(PI * time);
    }

	*this = (q1*scale) + (q2*invscale);
	return *this;
}


//! calculates the dot product
inline float CQuaternion::getDotProduct(const CQuaternion& q2) const
{
	return (X * q2.X) + (Y * q2.Y) + (Z * q2.Z) + (W * q2.W);
}


inline void CQuaternion::fromAngleAxis(float angle, const CVector3d<float>& axis)
{
	float fHalfAngle = 0.5f*angle;
	float fSin = (float)sin(fHalfAngle);
	W = (float)cos(fHalfAngle);
	X = fSin*axis.X;
	Y = fSin*axis.Y;
	Z = fSin*axis.Z;
}

inline void CQuaternion::toEuler(CVector3d<float>& euler)
{
	double sqw = W*W;
	double sqx = X*X;
	double sqy = Y*Y;
	double sqz = Z*Z;

	// heading = rotaton about z-axis
	euler.Z = (float) (atan2(2.0 * (X*Y +Z*W),(sqx - sqy - sqz + sqw)));

	// bank = rotation about x-axis
	euler.X = (float) (atan2(2.0 * (Y*Z +X*W),(-sqx - sqy + sqz + sqw)));

	// attitude = rotation about y-axis
	euler.Y = (float) (asin(-2.0 * (X*Z - Y*W)));
}

inline CVector3d<float> CQuaternion::operator* (const CVector3d<float>& v) const
{
	// nVidia SDK implementation

	CVector3d<float> uv, uuv;
	CVector3d<float> qvec(X, Y, Z);
	uv = qvec.crossProduct(v);
	uuv = qvec.crossProduct(uv);
	uv *= (2.0f * W);
	uuv *= 2.0f;

	return v + uv + uuv;
}

} // end namespace core
} // end namespace ox

#endif
