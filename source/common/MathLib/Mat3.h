/******************************************************************************
 Mat3
 *****************************************************************************/

#ifndef __MAT3_H__
#define __MAT3_H__


#pragma once

/*
================================================================================
Mat3
================================================================================
*/
template<class REAL>
class Mat3 {
public:

	Mat3() {}

	Vec3<REAL>					elem_[3]; // column major

	INLINE__ void				Identity();

	INLINE__					operator REAL * ();
	INLINE__					operator const REAL * ();

	INLINE__ const Vec3<REAL> &	operator [] (int idx) const;
	INLINE__ Vec3<REAL> &		operator [] (int idx);

	INLINE__ Mat3<REAL>			operator * (const Mat3<REAL> &other) const; // right multiply
	INLINE__ Vec3<REAL>			operator * (const Vec3<REAL> &v) const;

	INLINE__ REAL				Determinant() const;
	INLINE__ Mat3<REAL>			Transpose() const;
	INLINE__ Mat3<REAL>			TransposeSelf();
	INLINE__ Mat3<REAL>			Inverse() const;
	INLINE__ Mat3<REAL>			InverseSelf();
	INLINE__ void				Rotate(REAL degree, REAL x, REAL y, REAL z);
	INLINE__ void				Rotate(REAL degree, const Vec3<REAL> &axis);
	INLINE__ void				RotateAngleRad(REAL rad, REAL x, REAL y, REAL z);
	INLINE__ void				RotateAngleRad(REAL rad, const Vec3<REAL>& axis);
};

using Mat3f = Mat3<float>;
using Mat3d = Mat3<double>;

#include "Mat3.inl"

#endif //!__MAT3_H__
