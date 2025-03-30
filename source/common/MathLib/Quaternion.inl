/******************************************************************************
 Quaternion
 *****************************************************************************/

/*
================================================================================
Quaternion
================================================================================
*/
template<class REAL>
Quaternion<REAL>::Quaternion(REAL x, REAL y, REAL z, REAL w) :
	x_(x),
	y_(y),
	z_(z),
	w_(w)
{
}

template<class REAL>
INLINE__ void Quaternion<REAL>::Set(REAL v) {
	x_ = v;
	y_ = v;
	z_ = v;
	w_ = v;
}

template<class REAL>
INLINE__ void Quaternion<REAL>::Set(REAL x, REAL y) {
	x_ = x;
	y_ = y;
	z_ = (REAL)0.0;
	w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Quaternion<REAL>::Set(REAL x, REAL y, REAL z) {
	x_ = x;
	y_ = y;
	z_ = z;
	w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Quaternion<REAL>::Set(REAL x, REAL y, REAL z, REAL w) {
	x_ = x;
	y_ = y;
	z_ = z;
	w_ = w;
}

template<class REAL>
INLINE__ void Quaternion<REAL>::Zero() {
	x_ = y_ = z_ = w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ Quaternion<REAL>::operator REAL* () {
	return &x_;
}

template<class REAL>
INLINE__ Quaternion<REAL>::operator const REAL* () {
	return &x_;
}

template<class REAL>
INLINE__ const REAL& Quaternion<REAL>::operator [] (int idx) const {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ REAL& Quaternion<REAL>::operator [] (int idx) {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ Quaternion<REAL>& Quaternion<REAL>::operator = (const Quaternion& other) {
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
	w_ = other.w_;
	return *this;
}

template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::operator - () const {
	Quaternion<REAL> result;
	result.x_ = -x_;
	result.y_ = -y_;
	result.z_ = -z_;
	result.w_ = -w_;
	return result;
}

template<class REAL>
INLINE__ Quaternion<REAL>& Quaternion<REAL>::operator += (const Quaternion& other) {
	x_ += other.x_;
	y_ += other.y_;
	z_ += other.z_;
	w_ += other.w_;
	return *this;
}

template<class REAL>
INLINE__ Quaternion<REAL>& Quaternion<REAL>::operator -= (const Quaternion& other) {
	x_ -= other.x_;
	y_ -= other.y_;
	z_ -= other.z_;
	w_ -= other.w_;
	return *this;
}

template<class REAL>
INLINE__ Quaternion<REAL>& Quaternion<REAL>::operator *= (REAL f) {
	x_ *= f;
	y_ *= f;
	z_ *= f;
	w_ *= f;
	return *this;
}

template<class REAL>
INLINE__ Quaternion<REAL>& Quaternion<REAL>::operator /= (REAL f) {
	REAL i = Inv(f);
	x_ *= i;
	y_ *= i;
	z_ *= i;
	w_ *= i;
	return *this;
}

template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::operator + (const Quaternion<REAL>& other) const {
	Quaternion<REAL> result;
	result.x_ = x_ + other.x_;
	result.y_ = y_ + other.y_;
	result.z_ = z_ + other.z_;
	result.w_ = w_ + other.w_;
	return result;
}

template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::operator - (const Quaternion<REAL>& other) const {
	Quaternion<REAL> result;
	result.x_ = x_ - other.x_;
	result.y_ = y_ - other.y_;
	result.z_ = z_ - other.z_;
	result.w_ = w_ - other.w_;
	return result;
}

template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::operator * (REAL f) const {
	Quaternion<REAL> result;
	result.x_ = x_ * f;
	result.y_ = y_ * f;
	result.z_ = z_ * f;
	result.w_ = w_ * f;
	return result;
}

template<class REAL>
INLINE__ REAL Quaternion<REAL>::operator * (const Quaternion<REAL>& other) const {
	return x_ * other.x_ + y_ * other.y_ + z_ * other.z_ + w_ * other.w_;
}

template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::operator / (REAL f) const {
	REAL i = Inv(f);
	Vec4<REAL> result;
	result.x_ = x_ * i;
	result.y_ = y_ * i;
	result.z_ = z_ * i;
	result.w_ = w_ * i;
	return result;
}

template<class REAL>
INLINE__ int Quaternion<REAL>::operator == (const Quaternion<REAL>& other) const {
	return Compare(other);
}

template<class REAL>
INLINE__ int Quaternion<REAL>::operator != (const Quaternion<REAL>& other) const {
	return !Compare(other);
}

template<class REAL>
INLINE__ int Quaternion<REAL>::Compare(const Quaternion<REAL>& other) const {
	if (sizeof(REAL) == 8) {
		return (fabs(x_ - other.x_) < MATH_DBL_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_DBL_EPSILON)
			&& (fabs(z_ - other.z_) < MATH_DBL_EPSILON)
			&& (fabs(w_ - other.w_) < MATH_DBL_EPSILON);
	}
	else {
		return (fabs(x_ - other.x_) < MATH_FLT_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_FLT_EPSILON)
			&& (fabs(z_ - other.z_) < MATH_FLT_EPSILON)
			&& (fabs(w_ - other.w_) < MATH_FLT_EPSILON);
	}
}

template<class REAL>
INLINE__ void Quaternion<REAL>::FromAngle(const Angle<REAL> & angle) {
	REAL sr, sp, sy, cr, cp, cy;

	REAL a = angle[ANGLE_ROLL] * (REAL)0.5;
	sr = std::sin(a);
	cr = std::cos(a);
	a = angle[ANGLE_PITCH] * (REAL)0.5;
	sp = std::sin(a);
	cp = std::cos(a);
	a = angle[ANGLE_YAW] * (REAL)0.5;
	sy = std::sin(a);
	cy = std::cos(a);

	x_ = sr * cp * cy - cr * sp * sy;
	y_ = cr * sp * cy + sr * cp * sy;
	z_ = cr * cp * sy - sr * sp * cy;
	w_ = cr * cp * cy + sr * sp * sy;
}

template<class REAL>
INLINE__ void Quaternion<REAL>::ToAngle(Angle<REAL>& angle) const {
	Mat3<REAL> m;
	ToMatrix(m);

	REAL s = std::sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1]);

	REAL e = sizeof(REAL) == 8 ? MATH_DBL_EPSILON : MATH_FLT_EPSILON;

	if (s > e) {
		angle[ANGLE_PITCH] = RadToDeg(-std::atan2(m[0][2], s));
		angle[ANGLE_YAW] = RadToDeg(std::atan2(m[0][1], m[0][0]));
		angle[ANGLE_ROLL] = RadToDeg(std::atan2(m[1][2], m[2][2]));
	}
	else {
		angle[ANGLE_PITCH] = m[0][2] < (REAL)0.0 ? (REAL)90.0 : (REAL)-90.0;
		angle[ANGLE_YAW] = RadToDeg(-std::atan2(m[1][0], m[1][1]));
		angle[ANGLE_ROLL] = (REAL)0.0;
	}
}

template<class REAL>
INLINE__ void Quaternion<REAL>::FromMatrix(const Mat3<REAL>& m) {
	REAL		trace;
	REAL		s;
	REAL		t;
	int     	i;
	int			j;
	int			k;

	static int 	next[3] = { 1, 2, 0 };

	trace = m[0][0] + m[1][1] + m[2][2];

	if (trace > (REAL)0.0) {

		t = trace + (REAL)1.0;
		s = InvSqrt(t) * (REAL)0.5;

		w_ = s * t;
		x_ = (m[2][1] - m[1][2]) * s;
		y_ = (m[0][2] - m[2][0]) * s;
		z_ = (m[1][0] - m[0][1]) * s;

	}
	else {
		i = 0;
		if (m[1][1] > m[0][0]) {
			i = 1;
		}
		if (m[2][2] > m[i][i]) {
			i = 2;
		}
		j = next[i];
		k = next[j];

		t = (m[i][i] - (m[j][j] + m[k][k])) + (REAL)1.0;
		s = InvSqrt(t) * (REAL)0.5;

		(*this)[i] = s * t;
		(*this)[3] = (m[k][j] - m[j][k]) * s;
		(*this)[j] = (m[j][i] + m[i][j]) * s;
		(*this)[k] = (m[k][i] + m[i][k]) * s;
	}
}

template<class REAL>
INLINE__ void Quaternion<REAL>::ToMatrix(Mat3<REAL>& m) const {
	REAL xy2 = x_ * y_ * (REAL)2.0;
	REAL yz2 = y_ * z_ * (REAL)2.0;
	REAL xz2 = x_ * z_ * (REAL)2.0;
	REAL wx2 = w_ * x_ * (REAL)2.0;
	REAL wz2 = w_ * z_ * (REAL)2.0;
	REAL wy2 = w_ * y_ * (REAL)2.0;
	REAL xx2 = x_ * x_ * (REAL)2.0;
	REAL yy2 = y_ * y_ * (REAL)2.0;
	REAL zz2 = z_ * z_ * (REAL)2.0;

	m[0][0] = (REAL)1.0 - yy2 - zz2;
	m[0][1] = xy2 + wz2;
	m[0][2] = xz2 - wy2;

	m[1][0] = xy2 - wz2;
	m[1][1] = (REAL)1.0 - xx2 - zz2;
	m[1][2] = yz2 + wx2;

	m[2][0] = xz2 + wy2;
	m[2][1] = yz2 - wx2;
	m[2][2] = (REAL)1.0 - xx2 - yy2;
}

// spherical linear interpolation from a to b
template<class REAL>
INLINE__ Quaternion<REAL> Quaternion<REAL>::Interpolate(const Quaternion<REAL>& q1, const Quaternion<REAL>& q2, REAL t) {
	REAL omega, cosom, sinom, scale0, scale1;

	Quaternion<REAL> out(0.0, 0.0, 0.0, 0.0);

	if (t <= (REAL)0.0) {
		out = q1;
		return out;
	}

	if (t >= (REAL)1.0) {
		out = q2;
		return out;
	}

	if (q1 == q2) {
		out = q2;
		return out;
	}

	Quaternion<REAL> temp;
	cosom = q1.x_ * q2.x_ + q1.y_ * q2.y_ + q1.z_ * q2.z_ + q1.w_ * q2.w_;
	if (cosom < (REAL)0.0) {
		temp = -q2;
		cosom = -cosom;
	}
	else {
		temp = q2;
	}

	if (((REAL)1.0 - cosom) > (REAL)1e-6) {
		scale0 = (REAL)1.0 - cosom * cosom;
		sinom = InvSqrt(scale0);
		omega = std::atan2(scale0 * sinom, cosom);
		scale0 = std::sin(((REAL)1.0 - t) * omega) * sinom;
		scale1 = std::sin(t * omega) * sinom;
	}
	else {
		scale0 = (REAL)1.0 - t;
		scale1 = t;
	}

	out = (q1 * scale0) + (temp * scale1);

	return out;
}
