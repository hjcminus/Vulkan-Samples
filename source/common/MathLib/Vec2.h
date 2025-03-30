/******************************************************************************
 Vec2
 *****************************************************************************/

#ifndef __VEC2_H__
#define __VEC2_H__

#pragma once

/*
================================================================================
Vec2
================================================================================
*/
template<class REAL>
class Vec2 {
public:

	REAL						x_;
	REAL						y_;

	Vec2() {}
	Vec2(REAL x, REAL y);
	Vec2(const Vec2<REAL>& other);

	INLINE__ void				Set(REAL v);
	INLINE__ void				Set(REAL x, REAL y);
	INLINE__ void				Zero();

	INLINE__ 					operator REAL * ();
	INLINE__ 					operator const REAL * ();
	INLINE__ const REAL &		operator [] (int idx) const;
	INLINE__ REAL &				operator [] (int idx);
	INLINE__ Vec2<REAL> &		operator = (const Vec2<REAL> &other);
	INLINE__ Vec2<REAL>			operator - () const;
	INLINE__ Vec2<REAL> &		operator += (const Vec2<REAL> &other);
	INLINE__ Vec2<REAL> &		operator -= (const Vec2<REAL> &other);
	INLINE__ Vec2<REAL> &		operator *= (REAL f);
	INLINE__ Vec2<REAL> &		operator /= (REAL f);
	INLINE__ Vec2<REAL>			operator + (const Vec2<REAL> &other) const;
	INLINE__ Vec2<REAL>			operator - (const Vec2<REAL> &other) const;
	INLINE__ Vec2<REAL>			operator * (REAL f) const;
	INLINE__ REAL				operator * (const Vec2<REAL> &other) const;
	INLINE__ Vec2<REAL>			operator / (REAL f) const;
	INLINE__ int				operator == (const Vec2<REAL> &other) const;
	INLINE__ int				operator != (const Vec2<REAL> &other) const;

	INLINE__ int				Compare(const Vec2<REAL> &other) const;
	INLINE__ void				RotateSelf(REAL rad);
	INLINE__ REAL				Length() const;
	INLINE__ REAL				LengthSqr() const;
	INLINE__ REAL				Normalize();

	static INLINE__ Vec2<REAL>	Interpolate(const Vec2<REAL>& pt1, const Vec2<REAL>& pt2, REAL t);
	static INLINE__ Vec2<REAL>	ComponentMultiply(const Vec2<REAL>& a, const Vec2<REAL>& b);
	static INLINE__ Vec2<REAL>	ComponentMin(const Vec2<REAL>& a, const Vec2<REAL>& b);
	static INLINE__ Vec2<REAL>	ComponentMax(const Vec2<REAL>& a, const Vec2<REAL>& b);
};

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

#include "Vec2.inl"

#endif //!__VEC2_H__
