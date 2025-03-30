/******************************************************************************
 Angle
 *****************************************************************************/

#ifndef __ANGLE_H__
#define __ANGLE_H__

#pragma once

/*
================================================================================
Angle
================================================================================
*/
static constexpr int ANGLE_YAW = 0;
static constexpr int ANGLE_PITCH = 1;
static constexpr int ANGLE_ROLL = 2;

template<class REAL>
class Angle {
public:

	Angle() {}

	Vec3<REAL>					angles_;	// yaw-pitch-roll

	INLINE__ void				FromVector(const Vec3<REAL> & dir);
	INLINE__ void				ToVectors(Vec3<REAL> * vec_forward, Vec3<REAL> * vec_right, Vec3<REAL> * vec_up) const;
};

using Anglef = Angle<float>;
using Angled = Angle<double>;

#include "Angle.inl"

#endif //!__ANGLE_H__
