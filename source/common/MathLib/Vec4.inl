/******************************************************************************
 Vec4
 *****************************************************************************/

/*
================================================================================
Vec4
================================================================================
*/
template<class REAL>
Vec4<REAL>::Vec4(REAL x, REAL y, REAL z, REAL w) :
	x_(x),
	y_(y),
	z_(z),
	w_(w)
{
}

template<class REAL>
Vec4<REAL>::Vec4(const Vec4<REAL>& other) {
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
	w_ = other.w_;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(REAL v) {
	x_ = v;
	y_ = v;
	z_ = v;
	w_ = v;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(REAL x, REAL y) {
	x_ = x;
	y_ = y;
	z_ = (REAL)0.0;
	w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(REAL x, REAL y, REAL z) {
	x_ = x;
	y_ = y;
	z_ = z;
	w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(REAL x, REAL y, REAL z, REAL w) {
	x_ = x;
	y_ = y;
	z_ = z;
	w_ = w;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(const Vec2<REAL> &v) {
	x_ = v.x_;
	y_ = v.y_;
	z_ = 0.0f;
	w_ = 0.0f;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Set(const Vec3<REAL> &v) {
	x_ = v.x_;
	y_ = v.y_;
	z_ = v.z_;
	w_ = 0.0f;
}

template<class REAL>
INLINE__ void Vec4<REAL>::Zero() {
	x_ = y_ = z_ = w_ = (REAL)0.0;
}

template<class REAL>
INLINE__ Vec4<REAL>::operator REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ Vec4<REAL>::operator const REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ const REAL & Vec4<REAL>::operator [] (int idx) const {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ REAL & Vec4<REAL>::operator [] (int idx) {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ Vec4<REAL> & Vec4<REAL>::operator = (const Vec4 &other) {
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
	w_ = other.w_;
	return *this;
}

template<class REAL>
INLINE__ Vec4<REAL> Vec4<REAL>::operator - () const {
	Vec4<REAL> result;
	result.x_ = -x_;
	result.y_ = -y_;
	result.z_ = -z_;
	result.w_ = -w_;
	return result;
}

template<class REAL>
INLINE__ Vec4<REAL> & Vec4<REAL>::operator += (const Vec4 &other) {
	x_ += other.x_;
	y_ += other.y_;
	z_ += other.z_;
	w_ += other.w_;
	return *this;
}

template<class REAL>
INLINE__ Vec4<REAL> & Vec4<REAL>::operator -= (const Vec4 &other) {
	x_ -= other.x_;
	y_ -= other.y_;
	z_ -= other.z_;
	w_ -= other.w_;
	return *this;
}

template<class REAL>
INLINE__ Vec4<REAL> & Vec4<REAL>::operator *= (REAL f) {
	x_ *= f;
	y_ *= f;
	z_ *= f;
	w_ *= f;
	return *this;
}

template<class REAL>
INLINE__ Vec4<REAL> & Vec4<REAL>::operator /= (REAL f) {
	REAL i = Inv(f);
	x_ *= i;
	y_ *= i;
	z_ *= i;
	w_ *= i;
	return *this;
}

template<class REAL>
INLINE__ Vec4<REAL> Vec4<REAL>::operator + (const Vec4<REAL> &other) const {
	Vec4<REAL> result;
	result.x_ = x_ + other.x_;
	result.y_ = y_ + other.y_;
	result.z_ = z_ + other.z_;
	result.w_ = w_ + other.w_;
	return result;
}

template<class REAL>
INLINE__ Vec4<REAL> Vec4<REAL>::operator - (const Vec4<REAL> &other) const {
	Vec4<REAL> result;
	result.x_ = x_ - other.x_;
	result.y_ = y_ - other.y_;
	result.z_ = z_ - other.z_;
	result.w_ = w_ - other.w_;
	return result;
}

template<class REAL>
INLINE__ Vec4<REAL> Vec4<REAL>::operator * (REAL f) const {
	Vec4<REAL> result;
	result.x_ = x_ * f;
	result.y_ = y_ * f;
	result.z_ = z_ * f;
	result.w_ = w_ * f;
	return result;
}

template<class REAL>
INLINE__ REAL Vec4<REAL>::operator * (const Vec4<REAL> &other) const {
	return x_ * other.x_ + y_ * other.y_ + z_ * other.z_ + w_ * other.w_;
}

template<class REAL>
INLINE__ Vec4<REAL> Vec4<REAL>::operator / (REAL f) const {
	REAL i = Inv(f);
	Vec4<REAL> result;
	result.x_ = x_ * i;
	result.y_ = y_ * i;
	result.z_ = z_ * i;
	result.w_ = w_ * i;
	return result;
}

template<class REAL>
INLINE__ int Vec4<REAL>::operator == (const Vec4<REAL> &other) const {
	return Compare(other);
}

template<class REAL>
INLINE__ int Vec4<REAL>::operator != (const Vec4<REAL> &other) const {
	return !Compare(other);
}

template<class REAL>
INLINE__ int Vec4<REAL>::Compare(const Vec4<REAL> &other) const {
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
INLINE__ REAL Vec4<REAL>::Length() const {
	return sqrt(x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_);
}

template<class REAL>
INLINE__ REAL Vec4<REAL>::LengthSqr() const {
	return x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_;
}

template<class REAL>
INLINE__ REAL Vec4<REAL>::Normalize() {
	REAL l = Length();
	REAL i = Inv(l);
	x_ *= i;
	y_ *= i;
	z_ *= i;
	w_ *= i;
	return l; // return old length
}

template<class REAL>
INLINE__ Vec4<REAL>	Vec4<REAL>::Interpolate(const Vec4<REAL>& pt1, const Vec4<REAL>& pt2, REAL t) {
	Vec4<REAL> delta = pt2 - pt1;
	return pt1 + delta * t;
}

template<class REAL>
INLINE__ Vec4<REAL>	Vec4<REAL>::ComponentMultiply(const Vec4<REAL>& a, const Vec4<REAL>& b) {
	Vec4<REAL> v;
	v.x_ = a.x_ * b.x_;
	v.y_ = a.y_ * b.y_;
	v.z_ = a.z_ * b.z_;
	v.w_ = a.w_ * b.w_;
	return v;
}

template<class REAL>
INLINE__ Vec4<REAL>	Vec4<REAL>::ComponentMin(const Vec4<REAL>& a, const Vec4<REAL>& b) {
	Vec4<REAL> v;
	v.x_ = std::min(a.x_, b.x_);
	v.y_ = std::min(a.y_, b.y_);
	v.z_ = std::min(a.z_, b.z_);
	v.w_ = std::min(a.w_, b.w_);
	return v;
}

template<class REAL>
INLINE__ Vec4<REAL>	Vec4<REAL>::ComponentMax(const Vec4<REAL>& a, const Vec4<REAL>& b) {
	Vec4<REAL> v;
	v.x_ = std::max(a.x_, b.x_);
	v.y_ = std::max(a.y_, b.y_);
	v.z_ = std::max(a.z_, b.z_);
	v.w_ = std::max(a.w_, b.w_);
	return v;
}
