/******************************************************************************
 Vec3
 *****************************************************************************/

/*
================================================================================
Vec3
================================================================================
*/
template<class REAL>
Vec3<REAL>::Vec3(REAL x, REAL y, REAL z) :
	x_(x),
	y_(y),
	z_(z)
{
}

template<class REAL>
Vec3<REAL>::Vec3(const Vec3<REAL>& other) {
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
}

template<class REAL>
INLINE__ void Vec3<REAL>::Set(REAL v) {
	x_ = v;
	y_ = v;
	z_ = v;
}

template<class REAL>
INLINE__ void Vec3<REAL>::Set(REAL x, REAL y) {
	x_ = x;
	y_ = y;
	z_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Vec3<REAL>::Set(REAL x, REAL y, REAL z) {
	x_ = x;
	y_ = y;
	z_ = z;
}

template<class REAL>
INLINE__ void Vec3<REAL>::Set(const Vec2<REAL> &v) {
	x_ = v.x_;
	y_ = v.y_;
	z_ = (REAL)0.0;
}

template<class REAL>
INLINE__ void Vec3<REAL>::Zero() {
	x_ = y_ = z_ = (REAL)0.0;
}

template<class REAL>
INLINE__ Vec2< REAL> Vec3<REAL>::ToVec2() const {
	return Vec2<REAL>(x_, y_);
}

template<class REAL>
INLINE__ Vec3<REAL>::operator REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ Vec3<REAL>::operator const REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ const REAL & Vec3<REAL>::operator [] (int idx) const {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ REAL & Vec3<REAL>::operator [] (int idx) {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ Vec3<REAL> & Vec3<REAL>::operator = (const Vec3<REAL> &other) {
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
	return *this;
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::operator - () const {
	Vec3<REAL> result;
	result.x_ = -x_;
	result.y_ = -y_;
	result.z_ = -z_;
	return result;
}

template<class REAL>
INLINE__ Vec3<REAL> & Vec3<REAL>::operator += (const Vec3<REAL> &other) {
	x_ += other.x_;
	y_ += other.y_;
	z_ += other.z_;
	return *this;
}

template<class REAL>
INLINE__ Vec3<REAL> & Vec3<REAL>::operator -= (const Vec3<REAL> &other) {
	x_ -= other.x_;
	y_ -= other.y_;
	z_ -= other.z_;
	return *this;
}

template<class REAL>
INLINE__ Vec3<REAL> & Vec3<REAL>::operator *= (REAL f) {
	x_ *= f;
	y_ *= f;
	z_ *= f;
	return *this;
}

template<class REAL>
INLINE__ Vec3<REAL> & Vec3<REAL>::operator /= (REAL f) {
	REAL i = Inv(f);
	x_ *= i;
	y_ *= i;
	z_ *= i;
	return *this;
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::operator + (const Vec3 &other) const {
	Vec3<REAL> result;
	result.x_ = x_ + other.x_;
	result.y_ = y_ + other.y_;
	result.z_ = z_ + other.z_;
	return result;
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::operator - (const Vec3 &other) const {
	Vec3<REAL> result;
	result.x_ = x_ - other.x_;
	result.y_ = y_ - other.y_;
	result.z_ = z_ - other.z_;
	return result;
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::operator * (REAL f) const {
	Vec3<REAL> result;
	result.x_ = x_ * f;
	result.y_ = y_ * f;
	result.z_ = z_ * f;
	return result;
}

template<class REAL>
INLINE__ REAL Vec3<REAL>::operator * (const Vec3 &other) const {
	return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::operator / (REAL f) const {
	REAL i = Inv(f);
	Vec3 result;
	result.x_ = x_ * i;
	result.y_ = y_ * i;
	result.z_ = z_ * i;
	return result;
}

template<class REAL>
INLINE__ int Vec3<REAL>::operator == (const Vec3 &other) const {
	return Compare(other);
}

template<class REAL>
INLINE__ int Vec3<REAL>::operator != (const Vec3 &other) const {
	return !Compare(other);
}

template<class REAL>
INLINE__ int Vec3<REAL>::Compare(const Vec3 &other) const {
	if (sizeof(REAL) == 8) {
		return (fabs(x_ - other.x_) < MATH_DBL_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_DBL_EPSILON)
			&& (fabs(z_ - other.z_) < MATH_DBL_EPSILON);
	}
	else {
		return (fabs(x_ - other.x_) < MATH_FLT_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_FLT_EPSILON)
			&& (fabs(z_ - other.z_) < MATH_FLT_EPSILON);
	}
}

template<class REAL>
INLINE__ void Vec3<REAL>::RotateSelf(REAL rad, const Vec3<REAL>& axis) {
	Mat3<REAL> mat;
	mat.Rotate(rad, axis);

	Vec3<REAL> temp = *this;
	*this = mat * temp;
}

template<class REAL>
INLINE__ REAL Vec3<REAL>::Length() const {
	return sqrt(x_ * x_ + y_ * y_ + z_ * z_);
}

template<class REAL>
INLINE__ REAL Vec3<REAL>::LengthSqr() const {
	return x_ * x_ + y_ * y_ + z_ * z_;
}

template<class REAL>
INLINE__ REAL Vec3<REAL>::Normalize() {
	REAL l = Length();
	REAL i = Inv(l);
	x_ *= i;
	y_ *= i;
	z_ *= i;
	return l; // return old length
}

template<class REAL>
INLINE__ Vec3<REAL> Vec3<REAL>::CrossProduct(const Vec3<REAL> &a, const Vec3<REAL> &b) {
	Vec3<REAL> result;
	result.x_ = a.y_ * b.z_ - a.z_ * b.y_;
	result.y_ = a.z_ * b.x_ - a.x_ * b.z_;
	result.z_ = a.x_ * b.y_ - a.y_ * b.x_;
	return result;
}

template<class REAL>
INLINE__ Vec3<REAL>	Vec3<REAL>::Interpolate(const Vec3<REAL>& pt1, const Vec3<REAL>& pt2, REAL t) {
	Vec3<REAL> delta = pt2 - pt1;
	return pt1 + delta * t;
}

template<class REAL>
INLINE__ Vec3<REAL>	Vec3<REAL>::ComponentMultiply(const Vec3<REAL>& a, const Vec3<REAL>& b) {
	Vec3<REAL> v;
	v.x_ = a.x_ * b.x_;
	v.y_ = a.y_ * b.y_;
	v.z_ = a.z_ * b.z_;
	return v;
}

template<class REAL>
INLINE__ Vec3<REAL>	Vec3<REAL>::ComponentMin(const Vec3<REAL>& a, const Vec3<REAL>& b) {
	Vec3<REAL> v;
	v.x_ = std::min(a.x_, b.x_);
	v.y_ = std::min(a.y_, b.y_);
	v.z_ = std::min(a.z_, b.z_);
	return v;
}

template<class REAL>
INLINE__ Vec3<REAL>	Vec3<REAL>::ComponentMax(const Vec3<REAL>& a, const Vec3<REAL>& b) {
	Vec3<REAL> v;
	v.x_ = std::max(a.x_, b.x_);
	v.y_ = std::max(a.y_, b.y_);
	v.z_ = std::max(a.z_, b.z_);
	return v;
}
