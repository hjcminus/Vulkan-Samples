/******************************************************************************
 Vec2
 *****************************************************************************/

/*
================================================================================
Vec2
================================================================================
*/
template<class REAL>
Vec2<REAL>::Vec2(REAL x, REAL y):
x_(x),
y_(y)
{
}

template<class REAL>
Vec2<REAL>::Vec2(const Vec2<REAL>& other) {
	x_ = other.x_;
	y_ = other.y_;
}

template<class REAL>
INLINE__ void Vec2<REAL>::Set(REAL v) {
	x_ = v;
	y_ = v;
}

template<class REAL>
INLINE__ void Vec2<REAL>::Set(REAL x, REAL y) {
	x_ = x;
	y_ = y;
}

template<class REAL>
INLINE__ void Vec2<REAL>::Zero() {
	x_ = y_ = (REAL)0.0;
}

template<class REAL>
INLINE__ Vec2<REAL>::operator REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ Vec2<REAL>::operator const REAL * () {
	return &x_;
}

template<class REAL>
INLINE__ const REAL & Vec2<REAL>::operator [] (int idx) const {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ REAL & Vec2<REAL>::operator [] (int idx) {
	return (&x_)[idx];
}

template<class REAL>
INLINE__ Vec2<REAL> & Vec2<REAL>::operator = (const Vec2 &other) {
	x_ = other.x_;
	y_ = other.y_;
	return *this;
}

template<class REAL>
INLINE__ Vec2<REAL> Vec2<REAL>::operator - () const {
	Vec2 result;
	result.x_ = -x_;
	result.y = -y_;
	return result;
}

template<class REAL>
INLINE__ Vec2<REAL> & Vec2<REAL>::operator += (const Vec2 &other) {
	x_ += other.x_;
	y_ += other.y_;
	return *this;
}

template<class REAL>
INLINE__ Vec2<REAL> & Vec2<REAL>::operator -= (const Vec2 &other) {
	x_ -= other.x_;
	y_ -= other.y_;
	return *this;
}

template<class REAL>
INLINE__ Vec2<REAL> & Vec2<REAL>::operator *= (REAL f) {
	x_ *= f;
	y_ *= f;
	return *this;
}

template<class REAL>
INLINE__ Vec2<REAL> & Vec2<REAL>::operator /= (REAL f) {
	REAL i = Inv(f);
	x_ *= i;
	y_ *= i;
	return *this;
}

template<class REAL>
INLINE__ Vec2<REAL> Vec2<REAL>::operator + (const Vec2 &other) const {
	Vec2<REAL> result;
	result.x_ = x_ + other.x_;
	result.y_ = y_ + other.y_;
	return result;
}

template<class REAL>
INLINE__ Vec2<REAL> Vec2<REAL>::operator - (const Vec2 &other) const {
	Vec2<REAL> result;
	result.x_ = x_ - other.x_;
	result.y_ = y_ - other.y_;
	return result;
}

template<class REAL>
INLINE__ Vec2<REAL> Vec2<REAL>::operator * (REAL f) const {
	Vec2<REAL> result;
	result.x_ = x_ * f;
	result.y_ = y_ * f;
	return result;
}

template<class REAL>
INLINE__ REAL Vec2<REAL>::operator * (const Vec2 &other) const {
	return x_ * other.x_ + y_ * other.y_;
}

template<class REAL>
INLINE__ Vec2<REAL> Vec2<REAL>::operator / (REAL f) const {
	REAL i = Inv(f);
	Vec2 result;
	result.x_ = x_ * i;
	result.y_ = y_ * i;
	return result;
}

template<class REAL>
INLINE__ int Vec2<REAL>::operator == (const Vec2 &other) const {
	return Compare(other);
}

template<class REAL>
INLINE__ int Vec2<REAL>::operator != (const Vec2 &other) const {
	return !Compare(other);
}

template<class REAL>
INLINE__ int Vec2<REAL>::Compare(const Vec2 &other) const {
	if (sizeof(REAL) == 8) {
		return (fabs(x_ - other.x_) < MATH_DBL_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_DBL_EPSILON);
	}
	else {
		return (fabs(x_ - other.x_) < MATH_FLT_EPSILON)
			&& (fabs(y_ - other.y_) < MATH_FLT_EPSILON);
	}
}

template<class REAL>
INLINE__ void Vec2<REAL>::RotateSelf(REAL rad) {
	REAL cos_value = (REAL)cos(rad);
	REAL sin_value = (REAL)sin(rad);

	REAL new_x = x_ * cos_value - y_ * sin_value;
	REAL new_y = x_ * sin_value + y_ * cos_value;

	x_ = new_x;
	y_ = new_y;
}

template<class REAL>
INLINE__ REAL Vec2<REAL>::Length() const {
	return (REAL)sqrt(x_ * x_ + y_ * y_);
}

template<class REAL>
INLINE__ REAL Vec2<REAL>::LengthSqr() const {
	return x_ * x_ + y_ * y_;
}

template<class REAL>
INLINE__ REAL Vec2<REAL>::Normalize() {
	REAL l = Length();
	REAL i = Inv(l);
	x_ *= i;
	y_ *= i;
	return l; // return old length
}

template<class REAL>
INLINE__ Vec2<REAL>	Vec2<REAL>::Interpolate(const Vec2<REAL>& pt1, const Vec2<REAL>& pt2, REAL t) {
	Vec2<REAL> delta = pt2 - pt1;
	return pt1 + delta * t;
}

template<class REAL>
INLINE__ Vec2<REAL>	Vec2<REAL>::ComponentMultiply(const Vec2<REAL>& a, const Vec2<REAL>& b) {
	Vec2<REAL> v;
	v.x_ = a.x_ * b.x_;
	v.y_ = a.y_ * b.y_;
	return v;
}

template<class REAL>
INLINE__ Vec2<REAL>	Vec2<REAL>::ComponentMin(const Vec2<REAL>& a, const Vec2<REAL>& b) {
	Vec2<REAL> v;
	v.x_ = std::min(a.x_, b.x_);
	v.y_ = std::min(a.y_, b.y_);
	return v;
}

template<class REAL>
INLINE__ Vec2<REAL>	Vec2<REAL>::ComponentMax(const Vec2<REAL>& a, const Vec2<REAL>& b) {
	Vec2<REAL> v;
	v.x_ = std::max(a.x_, b.x_);
	v.y_ = std::max(a.y_, b.y_);
	return v;
}
