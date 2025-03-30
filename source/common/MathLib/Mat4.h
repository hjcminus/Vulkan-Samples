/******************************************************************************
 Mat4
 *****************************************************************************/

#ifndef __MAT4_H__
#define __MAT4_H__

#pragma once

/*
================================================================================
Mat4
================================================================================
*/
template<class REAL>
class Mat4 {
public:

	Mat4() {}

	Vec4<REAL>					elem_[4];	// column major

	INLINE__ void				Identity();

	INLINE__					operator REAL * ();
	INLINE__					operator const REAL * ();

	INLINE__ const Vec4<REAL> &	operator [] (int idx) const;
	INLINE__ Vec4<REAL> &		operator [] (int idx);

	INLINE__ Mat4<REAL>			operator * (const Mat4<REAL> &other) const; // right multiply
	INLINE__ Vec4<REAL>			operator * (const Vec4<REAL> &v) const;

	INLINE__ REAL				Determinant() const;
	INLINE__ Mat4<REAL>			Transpose() const;
	INLINE__ Mat4<REAL>			TransposeSelf();
	INLINE__ Mat4<REAL>			Inverse() const;
	INLINE__ Mat4<REAL>			InverseSelf();

	INLINE__ void				Translate(REAL dx, REAL dy, REAL dz);
	INLINE__ void				Translate(const Vec3<REAL> &delta);
	INLINE__ void				Rotate(REAL degree, REAL x, REAL y, REAL z);
	INLINE__ void				Rotate(REAL degree, const Vec3<REAL> &axis);
	INLINE__ void				RotateAngleRad(REAL rad, REAL x, REAL y, REAL z);
	INLINE__ void				RotateAngleRad(REAL rad, const Vec3<REAL>& axis);
	INLINE__ void				Scale(REAL sx, REAL sy, REAL sz);
	INLINE__ void				Scale(const Vec3<REAL> &scale);
	INLINE__ void				Scale(REAL s);
	INLINE__ void				Perspective(REAL fovy_degree, REAL aspect, REAL znear, REAL zfar);
	INLINE__ void				Frustum(REAL left, REAL right, REAL bottom, REAL top, REAL znear, REAL zfar);
	INLINE__ void				Ortho(REAL left, REAL right, REAL bottom, REAL top, REAL near_, REAL far_);
	INLINE__ void				LookAt(const Vec3<REAL> &eye, const Vec3<REAL> &center, const Vec3<REAL> &up);
};

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;

#include "Mat4.inl"

#endif //!__MAT4_H__
