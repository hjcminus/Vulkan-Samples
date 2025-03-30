/******************************************************************************
 Quaternion
 *****************************************************************************/

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#pragma once

/*
================================================================================
Quaternion
================================================================================
*/
template<class REAL>
class Quaternion {
public:

	REAL						x_;
	REAL						y_;
	REAL						z_;
	REAL						w_;

	Quaternion() {}
	Quaternion(REAL x, REAL y, REAL z, REAL w);

	INLINE__ void				Set(REAL v);
	INLINE__ void				Set(REAL x, REAL y);
	INLINE__ void				Set(REAL x, REAL y, REAL z);
	INLINE__ void				Set(REAL x, REAL y, REAL z, REAL w);
	INLINE__ void				Zero();

	INLINE__					operator REAL* ();
	INLINE__					operator const REAL* ();
	INLINE__ const REAL &		operator [] (int idx) const;
	INLINE__ REAL &				operator [] (int idx);
	INLINE__ Quaternion<REAL> & operator = (const Quaternion<REAL>& other);
	INLINE__ Quaternion<REAL>	operator - () const;
	INLINE__ Quaternion<REAL> & operator += (const Quaternion<REAL>& other);
	INLINE__ Quaternion<REAL> & operator -= (const Quaternion<REAL>& other);
	INLINE__ Quaternion<REAL> & operator *= (REAL f);
	INLINE__ Quaternion<REAL> & operator /= (REAL f);
	INLINE__ Quaternion<REAL>	operator + (const Quaternion<REAL>& other) const;
	INLINE__ Quaternion<REAL>	operator - (const Quaternion<REAL>& other) const;
	INLINE__ Quaternion<REAL>	operator * (REAL f) const;
	INLINE__ REAL				operator * (const Quaternion<REAL>& other) const;
	INLINE__ Quaternion<REAL>	operator / (REAL f) const;
	INLINE__ int				operator == (const Quaternion<REAL>& other) const;
	INLINE__ int				operator != (const Quaternion<REAL>& other) const;

	INLINE__ int				Compare(const Quaternion<REAL>& other) const;
	INLINE__ void				FromAngle(const Angle<REAL>& angle);
	INLINE__ void				ToAngle(Angle<REAL>& angle) const;
	INLINE__ void				FromMatrix(const Mat3<REAL>& m);
	INLINE__ void				ToMatrix(Mat3<REAL>& m) const;

	static INLINE__ Quaternion<REAL>	Interpolate(const Quaternion<REAL>& q1, const Quaternion<REAL>& q2, REAL t);
};

using Quaternionf = Quaternion<float>;
using Quaterniond = Quaternion<double>;

#include "Quaternion.inl"

#endif //!__QUATERNION_H__
