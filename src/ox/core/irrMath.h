// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/irrMath.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source.

#ifndef __IRR_MATH_H_INCLUDED__
#define __IRR_MATH_H_INCLUDED__


namespace ox
{
namespace core
{
	//! Rounding error constant often used when comparing float values.
	const float ROUNDING_ERROR	= 0.0001f;

	//! Constant for PI.
	const float PI				= 3.14159f;

	//! Constant for converting bettween degrees and radiants.
	const double GRAD_PI		= 180.0 / 3.1415926535897932384626433832795;

	//! Constant for converting bettween degrees and radiants.
	const double GRAD_PI2		= 3.1415926535897932384626433832795 / 180.0;

} // end namespace core
}// end namespace ox

#endif

