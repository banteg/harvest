// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SKeyMap.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __S_KEY_MAP_H_INCLUDED__
#define __S_KEY_MAP_H_INCLUDED__

#include "ox/Keycodes.h"

namespace daisy
{

	//! enumeration for key actions. Used for example in the FPS Camera.
	enum EKEY_ACTION
	{
		EKA_MOVE_FORWARD = 0,
		EKA_MOVE_BACKWARD,
		EKA_STRAFE_LEFT,
		EKA_STRAFE_RIGHT,
		EKA_COUNT,

		//! This value is not used. It only forces this enumeration to compile in 32 bit. 
		EKA_FORCE_32BIT = 0x7fffffff	
	};

	//! Struct storing which key belongs to which action.
	struct SKeyMap
	{
		EKEY_ACTION Action;
		ox::EKEY_CODE KeyCode;
	};

} // end namespace

#endif

