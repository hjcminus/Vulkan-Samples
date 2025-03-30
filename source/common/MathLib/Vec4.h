/******************************************************************************
 Vec4
 *****************************************************************************/

#ifndef __VEC4_H__
#define __VEC4_H__

#pragma once

/*
================================================================================
Vec4
================================================================================
*/
template<class REAL>
class Vec4 {
public:

	REAL						x_;
	REAL						y_;
	REAL						z_;
	REAL						w_;

	Vec4() {}
	Vec4(REAL x, REAL y, REAL z, REAL w);
	Vec4(const Vec4<REAL>& other);

	INLINE__ void				Set(REAL v);
	INLINE__ void				Set(REAL x, REAL y);
	INLINE__ void				Set(REAL x, REAL y, REAL z);
	INLINE__ void				Set(REAL x, REAL y, REAL z, REAL w);
	INLINE__ void				Set(const Vec2<REAL> &v);
	INLINE__ void				Set(const Vec3<REAL> &v);
	INLINE__ void				Zero();

	INLINE__					operator REAL * ();
	INLINE__					operator const REAL * ();
	INLINE__ const REAL &		operator [] (int idx) const;
	INLINE__ REAL &				operator [] (int idx);
	INLINE__ Vec4<REAL> &		operator = (const Vec4<REAL> &other);
	INLINE__ Vec4<REAL>			operator - () const;
	INLINE__ Vec4<REAL> &		operator += (const Vec4<REAL> &other);
	INLINE__ Vec4<REAL> &		operator -= (const Vec4<REAL> &other);
	INLINE__ Vec4<REAL> &		operator *= (REAL f);
	INLINE__ Vec4<REAL> &		operator /= (REAL f);
	INLINE__ Vec4<REAL>			operator + (const Vec4<REAL> &other) const;
	INLINE__ Vec4<REAL>			operator - (const Vec4<REAL> &other) const;
	INLINE__ Vec4<REAL>			operator * (REAL f) const;
	INLINE__ REAL				operator * (const Vec4<REAL> &other) const;
	INLINE__ Vec4<REAL>			operator / (REAL f) const;
	INLINE__ int				operator == (const Vec4<REAL> &other) const;
	INLINE__ int				operator != (const Vec4<REAL> &other) const;

	INLINE__ int				Compare(const Vec4<REAL> &other) const;
	INLINE__ REAL				Length() const;
	INLINE__ REAL				LengthSqr() const;
	INLINE__ REAL				Normalize();

	static INLINE__ Vec4<REAL>	Interpolate(const Vec4<REAL>& pt1, const Vec4<REAL>& pt2, REAL t);
	static INLINE__ Vec4<REAL>	ComponentMultiply(const Vec4<REAL>& a, const Vec4<REAL>& b);
	static INLINE__ Vec4<REAL>	ComponentMin(const Vec4<REAL>& a, const Vec4<REAL>& b);
	static INLINE__ Vec4<REAL>	ComponentMax(const Vec4<REAL>& a, const Vec4<REAL>& b);
};

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

#include "Vec4.inl"

#endif //!__VEC4_H__
