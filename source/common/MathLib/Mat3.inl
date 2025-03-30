/******************************************************************************
 Mat3
 *****************************************************************************/

/*
================================================================================
Mat3
================================================================================
*/
template<class REAL>
INLINE__ void Mat3<REAL>::Identity() {
	memset(elem_, 0, sizeof(elem_));
	elem_[0][0] = elem_[1][1] = elem_[2][2] = (REAL)1.0;
}

template<class REAL>
INLINE__ Mat3<REAL>::operator REAL * () {
	return &(elem_[0].x_);
}

template<class REAL>
INLINE__ Mat3<REAL>::operator const REAL * () {
	return &(elem_[0].x_);
}

template<class REAL>
INLINE__ const Vec3<REAL> & Mat3<REAL>::operator [] (int idx) const {
	return elem_[idx];
}

template<class REAL>
INLINE__ Vec3<REAL> & Mat3<REAL>::operator [] (int idx) {
	return elem_[idx];
}

template<class REAL>
INLINE__ Mat3<REAL> Mat3<REAL>::operator * (const Mat3<REAL> &other) const {
	Mat3<REAL> m;

	m.elem_[0][0] = elem_[0][0] * other.elem_[0][0] + elem_[1][0] * other.elem_[0][1] + elem_[2][0] * other.elem_[0][2];
	m.elem_[1][0] = elem_[0][0] * other.elem_[1][0] + elem_[1][0] * other.elem_[1][1] + elem_[2][0] * other.elem_[1][2];
	m.elem_[2][0] = elem_[0][0] * other.elem_[2][0] + elem_[1][0] * other.elem_[2][1] + elem_[2][0] * other.elem_[2][2];

	m.elem_[0][1] = elem_[0][1] * other.elem_[0][0] + elem_[1][1] * other.elem_[0][1] + elem_[2][1] * other.elem_[0][2];
	m.elem_[1][1] = elem_[0][1] * other.elem_[1][0] + elem_[1][1] * other.elem_[1][1] + elem_[2][1] * other.elem_[1][2];
	m.elem_[2][1] = elem_[0][1] * other.elem_[2][0] + elem_[1][1] * other.elem_[2][1] + elem_[2][1] * other.elem_[2][2];

	m.elem_[0][2] = elem_[0][2] * other.elem_[0][0] + elem_[1][2] * other.elem_[0][1] + elem_[2][2] * other.elem_[0][2];
	m.elem_[1][2] = elem_[0][2] * other.elem_[1][0] + elem_[1][2] * other.elem_[1][1] + elem_[2][2] * other.elem_[1][2];
	m.elem_[2][2] = elem_[0][2] * other.elem_[2][0] + elem_[1][2] * other.elem_[2][1] + elem_[2][2] * other.elem_[2][2];

	return m;
}

template<class REAL>
INLINE__ Vec3<REAL> Mat3<REAL>::operator * (const Vec3<REAL> &v) const {
	Vec3<REAL> r;

	r.x_ = elem_[0][0] * v.x_ + elem_[1][0] * v.y_ + elem_[2][0] * v.z_;
	r.y_ = elem_[0][1] * v.x_ + elem_[1][1] * v.y_ + elem_[2][1] * v.z_;
	r.z_ = elem_[0][2] * v.x_ + elem_[1][2] * v.y_ + elem_[2][2] * v.z_;

	return r;
}

template<class REAL>
INLINE__ REAL Mat3<REAL>::Determinant() const {
	// expand by first column
	REAL det0_0, det0_1, det0_2;

	det0_0 = elem_[1][1] * elem_[2][2] - elem_[2][1] * elem_[1][2];
	det0_1 = elem_[1][0] * elem_[2][2] - elem_[2][0] * elem_[1][2];
	det0_2 = elem_[1][0] * elem_[2][1] - elem_[2][0] * elem_[1][1];

	REAL d = elem_[0][0] * det0_0 - elem_[0][1] * det0_1 + elem_[0][2] * det0_2;

	return d;
}

template<class REAL>
INLINE__ Mat3<REAL> Mat3<REAL>::Transpose() const {
	Mat3 m;

	m.elem_[0][0] = elem_[0][0];
	m.elem_[0][1] = elem_[1][0];
	m.elem_[0][2] = elem_[2][0];

	m.elem_[1][0] = elem_[0][1];
	m.elem_[1][1] = elem_[1][1];
	m.elem_[1][2] = elem_[2][1];

	m.elem_[2][0] = elem_[0][2];
	m.elem_[2][1] = elem_[1][2];
	m.elem_[2][2] = elem_[2][2];

	return m;
}

template<class REAL>
INLINE__ Mat3<REAL> Mat3<REAL>::TransposeSelf() {
	Swap(elem_[0][1], elem_[1][0]);
	Swap(elem_[0][2], elem_[2][0]);
	Swap(elem_[1][2], elem_[2][1]);
	return *this;
}

template<class REAL>
INLINE__ Mat3<REAL> Mat3<REAL>::Inverse() const {
	Mat3<REAL> m;

	REAL d = Determinant();

	if (sizeof(REAL) == 8) {
		if (fabs(d) < MATH_DBL_EPSILON) {
			//D_ASSERT_ARG(false, L"singular matrix");
			m.Identity();
			return m;
		}
	}
	else {
		if (fabs(d) < MATH_FLT_EPSILON) {
			//D_ASSERT_ARG(false, L"singular matrix");
			m.Identity();
			return m;
		}
	}

	REAL invd = Inv(d);

#define DET2_(c1,c2,r1,r2) (elem_[c1][r1] * elem_[c2][r2] - elem_[c2][r1] * elem_[c1][r2])

	// column 0
	m.elem_[0][0] =  DET2_(1, 2, 1, 2) * invd;
	m.elem_[0][1] = -DET2_(1, 2, 0, 2) * invd;
	m.elem_[0][2] =  DET2_(1, 2, 0, 1) * invd;

	// column 1
	m.elem_[1][0] = -DET2_(0, 2, 1, 2) * invd;
	m.elem_[1][1] =  DET2_(0, 2, 0, 2) * invd;
	m.elem_[1][2] = -DET2_(0, 2, 0, 1) * invd;

	// column 2
	m.elem_[2][0] =  DET2_(0, 1, 1, 2) * invd;
	m.elem_[2][1] = -DET2_(0, 1, 0, 2) * invd;
	m.elem_[2][2] =  DET2_(0, 1, 0, 1) * invd;

	return m;
}

template<class REAL>
INLINE__ Mat3<REAL> Mat3<REAL>::InverseSelf() {
	*this = Inverse();
	return *this;
}

template<class REAL>
INLINE__ void Mat3<REAL>::Rotate(REAL degree, REAL x, REAL y, REAL z) {
	RotateAngleRad(DegToRad(degree), x, y, z);
}

template<class REAL>
INLINE__ void Mat3<REAL>::Rotate(REAL degree, const Vec3<REAL> &axis) {
	Rotate(degree, axis.x_, axis.y_, axis.z_);
}

template<class REAL>
INLINE__ void Mat3<REAL>::RotateAngleRad(REAL rad, REAL x, REAL y, REAL z) {
	Vec3<REAL> v;
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v.Normalize();

	x = v[0];
	y = v[1];
	z = v[2];

	REAL s = sinf(rad);
	REAL c = cosf(rad);

	REAL one_minus_c = (REAL)1.0 - c;

	REAL xy_one_minus_c = x * y * one_minus_c;
	REAL xz_one_minus_c = x * z * one_minus_c;
	REAL yz_one_minus_c = y * z * one_minus_c;

	REAL xs = x * s;
	REAL ys = y * s;
	REAL zs = z * s;

	elem_[0][0] = c + x * x * one_minus_c;
	elem_[0][1] = xy_one_minus_c + zs;
	elem_[0][2] = xz_one_minus_c - ys;

	elem_[1][0] = xy_one_minus_c - zs;
	elem_[1][1] = c + y * y * one_minus_c;
	elem_[1][2] = yz_one_minus_c + xs;

	elem_[2][0] = xz_one_minus_c + ys;
	elem_[2][1] = yz_one_minus_c - xs;
	elem_[2][2] = c + z * z * one_minus_c;
}

template<class REAL>
INLINE__ void Mat3<REAL>::RotateAngleRad(REAL rad, const Vec3<REAL>& axis) {
	RotateAngleRad(rad, axis.x_, axis.y_, axis.z_);
}
