/******************************************************************************
 Mat4
 *****************************************************************************/

/*
================================================================================
Mat4
================================================================================
*/
template<class REAL>
INLINE__ void Mat4<REAL>::Identity() {
	memset(elem_, 0, sizeof(elem_));
	elem_[0][0] = elem_[1][1] = elem_[2][2] = elem_[3][3] = 1.0f;
}

template<class REAL>
INLINE__ Mat4<REAL>::operator REAL * () {
	return &(elem_[0].x_);
}

template<class REAL>
INLINE__ Mat4<REAL>::operator const REAL * () {
	return &(elem_[0].x_);
}

template<class REAL>
INLINE__ const Vec4<REAL> & Mat4<REAL>::operator [] (int idx) const {
	return elem_[idx];
}

template<class REAL>
INLINE__ Vec4<REAL> & Mat4<REAL>::operator [] (int idx) {
	return elem_[idx];
}

template<class REAL>
INLINE__ Mat4<REAL> Mat4<REAL>::operator * (const Mat4<REAL> &other) const {
	Mat4<REAL> m;

	m.elem_[0][0] = elem_[0][0] * other.elem_[0][0] + elem_[1][0] * other.elem_[0][1] + elem_[2][0] * other.elem_[0][2] + elem_[3][0] * other.elem_[0][3];
	m.elem_[1][0] = elem_[0][0] * other.elem_[1][0] + elem_[1][0] * other.elem_[1][1] + elem_[2][0] * other.elem_[1][2] + elem_[3][0] * other.elem_[1][3];
	m.elem_[2][0] = elem_[0][0] * other.elem_[2][0] + elem_[1][0] * other.elem_[2][1] + elem_[2][0] * other.elem_[2][2] + elem_[3][0] * other.elem_[2][3];
	m.elem_[3][0] = elem_[0][0] * other.elem_[3][0] + elem_[1][0] * other.elem_[3][1] + elem_[2][0] * other.elem_[3][2] + elem_[3][0] * other.elem_[3][3];

	m.elem_[0][1] = elem_[0][1] * other.elem_[0][0] + elem_[1][1] * other.elem_[0][1] + elem_[2][1] * other.elem_[0][2] + elem_[3][1] * other.elem_[0][3];
	m.elem_[1][1] = elem_[0][1] * other.elem_[1][0] + elem_[1][1] * other.elem_[1][1] + elem_[2][1] * other.elem_[1][2] + elem_[3][1] * other.elem_[1][3];
	m.elem_[2][1] = elem_[0][1] * other.elem_[2][0] + elem_[1][1] * other.elem_[2][1] + elem_[2][1] * other.elem_[2][2] + elem_[3][1] * other.elem_[2][3];
	m.elem_[3][1] = elem_[0][1] * other.elem_[3][0] + elem_[1][1] * other.elem_[3][1] + elem_[2][1] * other.elem_[3][2] + elem_[3][1] * other.elem_[3][3];

	m.elem_[0][2] = elem_[0][2] * other.elem_[0][0] + elem_[1][2] * other.elem_[0][1] + elem_[2][2] * other.elem_[0][2] + elem_[3][2] * other.elem_[0][3];
	m.elem_[1][2] = elem_[0][2] * other.elem_[1][0] + elem_[1][2] * other.elem_[1][1] + elem_[2][2] * other.elem_[1][2] + elem_[3][2] * other.elem_[1][3];
	m.elem_[2][2] = elem_[0][2] * other.elem_[2][0] + elem_[1][2] * other.elem_[2][1] + elem_[2][2] * other.elem_[2][2] + elem_[3][2] * other.elem_[2][3];
	m.elem_[3][2] = elem_[0][2] * other.elem_[3][0] + elem_[1][2] * other.elem_[3][1] + elem_[2][2] * other.elem_[3][2] + elem_[3][2] * other.elem_[3][3];

	m.elem_[0][3] = elem_[0][3] * other.elem_[0][0] + elem_[1][3] * other.elem_[0][1] + elem_[2][3] * other.elem_[0][2] + elem_[3][3] * other.elem_[0][3];
	m.elem_[1][3] = elem_[0][3] * other.elem_[1][0] + elem_[1][3] * other.elem_[1][1] + elem_[2][3] * other.elem_[1][2] + elem_[3][3] * other.elem_[1][3];
	m.elem_[2][3] = elem_[0][3] * other.elem_[2][0] + elem_[1][3] * other.elem_[2][1] + elem_[2][3] * other.elem_[2][2] + elem_[3][3] * other.elem_[2][3];
	m.elem_[3][3] = elem_[0][3] * other.elem_[3][0] + elem_[1][3] * other.elem_[3][1] + elem_[2][3] * other.elem_[3][2] + elem_[3][3] * other.elem_[3][3];

	return m;
}

template<class REAL>
INLINE__ Vec4<REAL> Mat4<REAL>::operator * (const Vec4<REAL> &v) const {
	Vec4<REAL> r;

	r.x_ = elem_[0][0] * v.x_ + elem_[1][0] * v.y_ + elem_[2][0] * v.z_ + elem_[3][0] * v.w_;
	r.y_ = elem_[0][1] * v.x_ + elem_[1][1] * v.y_ + elem_[2][1] * v.z_ + elem_[3][1] * v.w_;
	r.z_ = elem_[0][2] * v.x_ + elem_[1][2] * v.y_ + elem_[2][2] * v.z_ + elem_[3][2] * v.w_;
	r.w_ = elem_[0][3] * v.x_ + elem_[1][3] * v.y_ + elem_[2][3] * v.z_ + elem_[3][3] * v.w_;

	return r;
}

template<class REAL>
INLINE__ REAL Mat4<REAL>::Determinant() const {
	// expand by first column
	REAL det0_0, det0_1, det0_2, det0_3;

	// 36 multiply

	REAL    e_2__2__M_e_3__3_ = elem_[2][2] * elem_[3][3];
	REAL    e_3__2__M_e_1__3_ = elem_[3][2] * elem_[1][3];
	REAL    e_1__2__M_e_2__3_ = elem_[1][2] * elem_[2][3];

	REAL    e_2__2__M_e_1__3_ = elem_[2][2] * elem_[1][3];
	REAL    e_1__2__M_e_3__3_ = elem_[1][2] * elem_[3][3];
	REAL    e_3__2__M_e_2__3_ = elem_[3][2] * elem_[2][3];

	REAL    e_1__0__M_e_2__1_ = elem_[1][0] * elem_[2][1];
	REAL    e_2__0__M_e_3__1_ = elem_[2][0] * elem_[3][1];
	REAL    e_3__0__M_e_1__1_ = elem_[3][0] * elem_[1][1];

	REAL    e_3__0__M_e_2__1_ = elem_[3][0] * elem_[2][1];
	REAL    e_2__0__M_e_1__1_ = elem_[2][0] * elem_[1][1];
	REAL    e_1__0__M_e_3__1_ = elem_[1][0] * elem_[3][1];

	det0_0 = elem_[1][1] * e_2__2__M_e_3__3_ + elem_[2][1] * e_3__2__M_e_1__3_ + elem_[3][1] * e_1__2__M_e_2__3_
		- elem_[3][1] * e_2__2__M_e_1__3_ - elem_[2][1] * e_1__2__M_e_3__3_ - elem_[1][1] * e_3__2__M_e_2__3_;

	det0_1 = elem_[1][0] * e_2__2__M_e_3__3_ + elem_[2][0] * e_3__2__M_e_1__3_ + elem_[3][0] * e_1__2__M_e_2__3_
		- elem_[3][0] * e_2__2__M_e_1__3_ - elem_[2][0] * e_1__2__M_e_3__3_ - elem_[1][0] * e_3__2__M_e_2__3_;

	det0_2 = e_1__0__M_e_2__1_ * elem_[3][3] + e_2__0__M_e_3__1_ * elem_[1][3] + e_3__0__M_e_1__1_ * elem_[2][3]
		- e_3__0__M_e_2__1_ * elem_[1][3] - e_2__0__M_e_1__1_ * elem_[3][3] - e_1__0__M_e_3__1_ * elem_[2][3];

	det0_3 = e_1__0__M_e_2__1_ * elem_[3][2] + e_2__0__M_e_3__1_ * elem_[1][2] + e_3__0__M_e_1__1_ * elem_[2][2]
		- e_3__0__M_e_2__1_ * elem_[1][2] - e_2__0__M_e_1__1_ * elem_[3][2] - e_1__0__M_e_3__1_ * elem_[2][2];


	REAL d = elem_[0][0] * det0_0 - elem_[0][1] * det0_1 + elem_[0][2] * det0_2 - elem_[0][3] * det0_3;

	return d;
}

template<class REAL>
INLINE__ Mat4<REAL> Mat4<REAL>::Transpose() const {
	Mat4<REAL> m;

	m.elem_[0][0] = elem_[0][0];
	m.elem_[0][1] = elem_[1][0];
	m.elem_[0][2] = elem_[2][0];
	m.elem_[0][3] = elem_[3][0];

	m.elem_[1][0] = elem_[0][1];
	m.elem_[1][1] = elem_[1][1];
	m.elem_[1][2] = elem_[2][1];
	m.elem_[1][3] = elem_[3][1];

	m.elem_[2][0] = elem_[0][2];
	m.elem_[2][1] = elem_[1][2];
	m.elem_[2][2] = elem_[2][2];
	m.elem_[2][3] = elem_[3][2];

	m.elem_[3][0] = elem_[0][3];
	m.elem_[3][1] = elem_[1][3];
	m.elem_[3][2] = elem_[2][3];
	m.elem_[3][3] = elem_[3][3];

	return m;
}

template<class REAL>
INLINE__ Mat4<REAL> Mat4<REAL>::TransposeSelf() {
	Swap(elem_[0][1], elem_[1][0]);
	Swap(elem_[0][2], elem_[2][0]);
	Swap(elem_[0][3], elem_[3][0]);
	Swap(elem_[1][2], elem_[2][1]);
	Swap(elem_[1][3], elem_[3][1]);
	Swap(elem_[2][3], elem_[3][2]);
	return *this;
}

template<class REAL>
INLINE__ Mat4<REAL> Mat4<REAL>::Inverse() const {
#define SWAP_ROWS(a, b) { REAL *_tmp = a; (a) = (b); (b) = _tmp; }
#define MAT(m, r, c) (m)[(c)* 4 + (r)]

	const REAL *m = &elem_[0][0];

	Mat4<REAL> o;
	REAL *out = &o.elem_[0][0];

	REAL wtmp[4][8];
	REAL m0, m1, m2, m3, s;
	REAL *r0, *r1, *r2, *r3;
	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
		r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
		r0[4] = (REAL)1.0, r0[5] = r0[6] = r0[7] = (REAL)0.0,
		r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
		r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
		r1[5] = (REAL)1.0, r1[4] = r1[6] = r1[7] = (REAL)0.0,
		r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
		r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
		r2[6] = (REAL)1.0, r2[4] = r2[5] = r2[7] = (REAL)0.0,
		r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
		r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
		r3[7] = (REAL)1.0, r3[4] = r3[5] = r3[6] = (REAL)0.0;

	/* choose pivot - or die */
	if (fabs(r3[0]) > fabs(r2[0]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[0]) > fabs(r1[0]))
		SWAP_ROWS(r2, r1);
	if (fabs(r1[0]) > fabs(r0[0]))
		SWAP_ROWS(r1, r0);
	if ((REAL)0.0 == r0[0])
		return o;
	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != (REAL)0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != (REAL)0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != (REAL)0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0f) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabs(r3[1]) > fabs(r2[1]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[1]) > fabs(r1[1]))
		SWAP_ROWS(r2, r1);
	if ((REAL)0.0 == r1[1])
		return o;
	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if ((REAL)0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if ((REAL)0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if ((REAL)0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if ((REAL)0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabs(r3[2]) > fabs(r2[2]))
		SWAP_ROWS(r3, r2);
	if ((REAL)0.0 == r2[2])
		return o;
	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
	/* last check */
	if ((REAL)0.0 == r3[3])
		return o;
	s = (REAL)1.0 / r3[3];		/* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;
	m2 = r2[3];			/* now back substitute row 2 */
	s = (REAL)1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
		r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
		r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
	m1 = r1[2];			/* now back substitute row 1 */
	s = (REAL)1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
		r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
	m0 = r0[1];			/* now back substitute row 0 */
	s = (REAL)1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

	return o;
}

template<class REAL>
INLINE__ Mat4<REAL> Mat4<REAL>::InverseSelf() {
	*this = Inverse();
	return *this;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Translate(REAL dx, REAL dy, REAL dz) {
	Identity();
	elem_[3][0] = dx;
	elem_[3][1] = dy;
	elem_[3][2] = dz;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Translate(const Vec3<REAL> &delta) {
	Translate(delta.x_, delta.y_, delta.z_);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Rotate(REAL degree, REAL x, REAL y, REAL z) {
	Vec3<REAL> axis = { x, y, z };
	Rotate(degree, axis);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Rotate(REAL degree, const Vec3<REAL> &axis) {
	RotateAngleRad(DegToRad(degree), axis);
}

template<class REAL>
INLINE__ void Mat4<REAL>::RotateAngleRad(REAL rad, REAL x, REAL y, REAL z) {
	RotateAngleRad(rad, Vec3<REAL>(x, y, z));
}

template<class REAL>
INLINE__ void Mat4<REAL>::RotateAngleRad(REAL rad, const Vec3<REAL>& axis) {
	Vec3<REAL> v = axis;
	v.Normalize();

	REAL x = v[0];
	REAL y = v[1];
	REAL z = v[2];

	REAL s = sin(rad);
	REAL c = cos(rad);

	REAL one_minus_c = 1.0f - c;

	REAL xy_one_minus_c = x * y * one_minus_c;
	REAL xz_one_minus_c = x * z * one_minus_c;
	REAL yz_one_minus_c = y * z * one_minus_c;

	REAL xs = x * s;
	REAL ys = y * s;
	REAL zs = z * s;

	elem_[0][0] = c + x * x * one_minus_c;
	elem_[0][1] = xy_one_minus_c + zs;
	elem_[0][2] = xz_one_minus_c - ys;
	elem_[0][3] = (REAL)0.0;

	elem_[1][0] = xy_one_minus_c - zs;
	elem_[1][1] = c + y * y * one_minus_c;
	elem_[1][2] = yz_one_minus_c + xs;
	elem_[1][3] = (REAL)0.0;

	elem_[2][0] = xz_one_minus_c + ys;
	elem_[2][1] = yz_one_minus_c - xs;
	elem_[2][2] = c + z * z * one_minus_c;
	elem_[2][3] = (REAL)0.0;

	elem_[3][0] = (REAL)0.0;
	elem_[3][1] = (REAL)0.0;
	elem_[3][2] = (REAL)0.0;
	elem_[3][3] = (REAL)1.0;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Scale(REAL sx, REAL sy, REAL sz) {
	memset(&elem_, 0, sizeof(elem_));
	elem_[0][0] = sx;
	elem_[1][1] = sy;
	elem_[2][2] = sz;
	elem_[3][3] = (REAL)1.0;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Scale(const Vec3<REAL> &scale) {
	Scale(scale.x_, scale.y_, scale.z_);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Scale(REAL s) {
	Scale(s, s, s);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Perspective(REAL fovy_degree, REAL aspect, REAL znear, REAL zfar) {
	REAL t = tan(DegToRad(fovy_degree) * (REAL)0.5);

	REAL tp = t * znear;
	REAL rt = tp * aspect;

	Frustum(-rt, rt, -tp, tp, znear, zfar);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Perspective_ZO(REAL fovy_degree, REAL aspect, REAL znear, REAL zfar) {
	REAL t = tan(DegToRad(fovy_degree) * (REAL)0.5);

	REAL tp = t * znear;
	REAL rt = tp * aspect;

	Frustum_ZO(-rt, rt, -tp, tp, znear, zfar);
}

template<class REAL>
INLINE__ void Mat4<REAL>::Frustum(REAL left, REAL right, REAL bottom, REAL top, REAL znear, REAL zfar) {
	memset(&elem_, 0, sizeof(elem_));

	REAL r_l = right - left;
	REAL r_add_l = right + left;
	REAL t_add_b = top + bottom;
	REAL t_b = top - bottom;
	REAL _2n = (REAL)2.0 * znear;
	REAL f_add_n = zfar + znear;
	REAL f_n = zfar - znear;

	REAL inv_f_n = Inv(f_n);

	elem_[0][0] = _2n / r_l;
	elem_[1][1] = _2n / t_b;
	elem_[2][0] = r_add_l / r_l;
	elem_[2][1] = t_add_b / t_b;
	elem_[2][2] = -f_add_n * inv_f_n;
	elem_[2][3] = (REAL)-1.0;
	elem_[3][2] = (REAL)-2.0 * znear * zfar * inv_f_n;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Frustum_ZO(REAL left, REAL right, REAL bottom, REAL top, REAL znear, REAL zfar) {
	memset(&elem_, 0, sizeof(elem_));

	REAL r_l = right - left;
	REAL r_add_l = right + left;
	REAL t_add_b = top + bottom;
	REAL t_b = top - bottom;
	REAL _2n = (REAL)2.0 * znear;
	REAL f_add_n = zfar + znear;
	REAL f_n = zfar - znear;

	REAL inv_f_n = Inv(f_n);

	elem_[0][0] = _2n / r_l;
	elem_[1][1] = _2n / t_b;
	elem_[2][0] = r_add_l / r_l;
	elem_[2][1] = t_add_b / t_b;
	elem_[2][2] = -zfar * inv_f_n;
	elem_[2][3] = (REAL)-1.0;
	elem_[3][2] = (REAL)-znear * zfar * inv_f_n;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Ortho(REAL left, REAL right, REAL bottom, REAL top, REAL near_, REAL far_) {
	REAL r_l = right - left;
	REAL t_b = top - bottom;
	REAL f_n = far_ - near_;

	memset(&elem_, 0, sizeof(elem_));

	elem_[0][0] = (REAL)2.0 / r_l;
	elem_[1][1] = (REAL)2.0 / t_b;
	elem_[2][2] = (REAL)-2.0 / f_n;
	elem_[3][0] = -(right + left) / r_l;
	elem_[3][1] = -(top + bottom) / t_b;
	elem_[3][2] = -(far_ + near_) / f_n;
	elem_[3][3] = (REAL)1.0;
}

template<class REAL>
INLINE__ void Mat4<REAL>::Ortho_ZO(REAL left, REAL right, REAL bottom, REAL top, REAL near_, REAL far_) {
	REAL r_l = right - left;
	REAL t_b = top - bottom;
	REAL f_n = far_ - near_;

	memset(&elem_, 0, sizeof(elem_));

	elem_[0][0] = (REAL)2.0 / r_l;
	elem_[1][1] = (REAL)2.0 / t_b;
	elem_[2][2] = (REAL)-1.0 / f_n;
	elem_[3][0] = -(right + left) / r_l;
	elem_[3][1] = -(top + bottom) / t_b;
	elem_[3][2] = -(near_) / f_n;
	elem_[3][3] = (REAL)1.0;
}

template<class REAL>
INLINE__ void Mat4<REAL>::LookAt(const Vec3<REAL> &eye, const Vec3<REAL> &center, const Vec3<REAL> &up) {
	Vec3<REAL> forward = center - eye;
	forward.Normalize();

	Vec3<REAL> up_ = up;
	up_.Normalize();

	Vec3<REAL> side = Vec3<REAL>::CrossProduct(forward, up_);

	elem_[0][0] = side.x_;
	elem_[1][0] = side.y_;
	elem_[2][0] = side.z_;
	elem_[3][0] = -(side * eye);

	elem_[0][1] = up_.x_;
	elem_[1][1] = up_.y_;
	elem_[2][1] = up_.z_;
	elem_[3][1] = -(up_ * eye);

	elem_[0][2] = -forward.x_;
	elem_[1][2] = -forward.y_;
	elem_[2][2] = -forward.z_;
	elem_[3][2] = forward * eye;

	elem_[0][3] = elem_[1][3] = elem_[2][3] = (REAL)0.0;
	elem_[3][3] = (REAL)1.0;
}
