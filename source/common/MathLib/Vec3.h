/******************************************************************************
 Vec3
 *****************************************************************************/

#ifndef __VEC3_H__
#define __VEC3_H__

#pragma once

/*
================================================================================
Vec3
================================================================================
*/
template<class REAL>
class Vec3 {
public:

	REAL						x_;
	REAL						y_;
	REAL						z_;

	Vec3() {}
	Vec3(REAL x, REAL y, REAL z);
	Vec3(const Vec3<REAL> & other);

	INLINE__ void				Set(REAL v);
	INLINE__ void				Set(REAL x, REAL y);
	INLINE__ void				Set(REAL x, REAL y, REAL z);
	INLINE__ void				Set(const Vec2<REAL> &v);
	INLINE__ void				Zero();

	INLINE__ Vec2<REAL>			ToVec2() const;

	INLINE__					operator REAL * ();
	INLINE__					operator const REAL * ();
	INLINE__ const REAL &		operator [] (int idx) const;
	INLINE__ REAL &				operator [] (int idx);
	INLINE__ Vec3<REAL> &		operator = (const Vec3<REAL> &other);
	INLINE__ Vec3<REAL>			operator - () const;
	INLINE__ Vec3<REAL> &		operator += (const Vec3<REAL> &other);
	INLINE__ Vec3<REAL> &		operator -= (const Vec3<REAL> &other);
	INLINE__ Vec3<REAL> &		operator *= (REAL f);
	INLINE__ Vec3<REAL> &		operator /= (REAL f);
	INLINE__ Vec3<REAL>			operator + (const Vec3<REAL> &other) const;
	INLINE__ Vec3<REAL>			operator - (const Vec3<REAL> &other) const;
	INLINE__ Vec3<REAL>			operator * (REAL f) const;
	INLINE__ REAL				operator * (const Vec3 &other) const;
	INLINE__ Vec3<REAL>			operator / (REAL f) const;
	INLINE__ int				operator == (const Vec3<REAL> &other) const;
	INLINE__ int				operator != (const Vec3<REAL> &other) const;

	INLINE__ int				Compare(const Vec3<REAL> &other) const;
	INLINE__ void				RotateSelf(REAL rad, const Vec3<REAL>& axis);
	INLINE__ REAL				Length() const;
	INLINE__ REAL				LengthSqr() const;
	INLINE__ REAL				Normalize();

	static INLINE__ Vec3<REAL>	CrossProduct(const Vec3<REAL> &a, const Vec3<REAL> &b);
	static INLINE__ Vec3<REAL>	Interpolate(const Vec3<REAL>& pt1, const Vec3<REAL>& pt2, REAL t);
	static INLINE__ Vec3<REAL>	ComponentMultiply(const Vec3<REAL>& a, const Vec3<REAL>& b);
	static INLINE__ Vec3<REAL>	ComponentMin(const Vec3<REAL>& a, const Vec3<REAL>& b);
	static INLINE__ Vec3<REAL>	ComponentMax(const Vec3<REAL>& a, const Vec3<REAL>& b);
};

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

// #include "Vec3.inl"

#endif //!__VEC3_H__
