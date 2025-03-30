/******************************************************************************
 MathLib
 *****************************************************************************/

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#pragma once

#include <stdint.h>
#include <cmath>

#if defined(_MSC_VER)
# define INLINE__	__forceinline
#endif

#if defined(__GNUC__)
//# define INLINE__	__attribute__((__always_inline__))
# define INLINE__   inline
#endif

#define MATH_FLT_IEEE_MANTISSA_BITS 23

static const int32_t	MATH_FLT_SMALLEST_NON_DENORMAL_BITS = 1 << MATH_FLT_IEEE_MANTISSA_BITS;
static const float		MATH_FLT_INFINITY = 1e30f;
static const float		MATH_FLT_EPSILON = 1.192092896e-07f; // float.h
static const float		MATH_FLT_SMALLEST_NON_DENORMAL = *(const float *)(&MATH_FLT_SMALLEST_NON_DENORMAL_BITS);

#define MATH_DBL_IEEE_MANTISSA_BITS 52

static const int64_t	MATH_DBL_SMALLEST_NON_DENORMAL_BITS = 1LL << MATH_DBL_IEEE_MANTISSA_BITS;
static const double		MATH_DBL_INFINITY = 1e+300;
static const double		MATH_DBL_EPSILON = 2.2204460492503131e-016; // float.h
static const double		MATH_DBL_SMALLEST_NON_DENORMAL = *(const double*)(&MATH_DBL_SMALLEST_NON_DENORMAL_BITS);

#define MATH_PI			3.1415926535897932346
#define MATH_2PI		6.2831853071795864692
#define MATH_PI_OVER_2  1.5707963267948966173
#define MATH_PI_OVER_4	0.7853981633974483087

#define SQUARE(X)		((X) * (X))

static INLINE__	float	Inv(float v);
static INLINE__	double	Inv(double v);

static INLINE__	bool	NaN(float v);
static INLINE__	bool	NaN(double v);

static INLINE__	float	DegToRad(float d);
static INLINE__	float	RadToDeg(float r);

static INLINE__	double	DegToRad(double d);
static INLINE__	double	RadToDeg(double r);

#if defined(_MSC_VER)
static INLINE__ float	NormalizeRad(float r);
#endif
static INLINE__ double  NormalizeRad(double r);

// 2021.06.07 Mon.
static INLINE__ float	AngleBetween(float dir1, float dir2);
static INLINE__ double	AngleBetween(double dir1, double dir2);


static INLINE__ bool	RangeOverlap(float range1_min, float range1_max, float range2_min, float range2_max);
static INLINE__ bool	RangeOverlap(double range1_min, double range1_max, double range2_min, double range2_max);


template<class T>
T clamp_(T v, T min_, T max_) {
	if (v < min_) {
		return min_;
	}

	if (v > max_) {
		return max_;
	}

	return v;
}

#include "MathLib.inl"

#endif //!__MATHLIB_H__
