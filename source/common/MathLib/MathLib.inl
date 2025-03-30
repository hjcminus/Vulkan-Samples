/******************************************************************************
 MathLib
 *****************************************************************************/

INLINE__ float Inv(float v) {
	return (fabs(v) > MATH_FLT_SMALLEST_NON_DENORMAL) ? 1 / v : MATH_FLT_INFINITY;
}

INLINE__ double Inv(double v) {
	return (fabs(v) > MATH_DBL_SMALLEST_NON_DENORMAL) ? 1 / v : MATH_DBL_INFINITY;
}

INLINE__ bool NaN(float v) {
	return v != v;
}

INLINE__ bool NaN(double v) {
	return v != v;
}

INLINE__ float DegToRad(float d) {
	return d * (float)MATH_PI / 180.0f;
}

INLINE__ float RadToDeg(float r) {
	return r * 180.0f / (float)MATH_PI;
}

INLINE__ double	DegToRad(double d) {
	return d * MATH_PI / 180.0;
}

INLINE__ double	RadToDeg(double r) {
	return r * 180.0 / MATH_PI;
}

#if defined(_MSC_VER)
static INLINE__ float NormalizeRad(float r) {
	float a = std::fmodf(r + (float)MATH_PI, 2.0f * (float)MATH_PI);
	if (a < 0.0f) {
		a += (2.0f * (float)MATH_PI);
	}
	return a - (float)MATH_PI;
}
#endif

INLINE__ double  NormalizeRad(double r) {
    double a = std::fmod(r + MATH_PI, 2.0 * MATH_PI);
    if (a < 0.0) {
        a += (2.0 * MATH_PI);
    }
    return a - MATH_PI;
}

// 2021.06.07 Mon.
INLINE__ float AngleBetween(float dir1, float dir2) {
	float x1 = cosf(dir1);
	float y1 = sinf(dir1);
	float x2 = cosf(dir2);
	float y2 = sinf(dir2);
	float dot = x1 * x2 + y1 * y2;
	float angle = acosf(dot);
	return angle;
}

INLINE__ double	AngleBetween(double dir1, double dir2) {
	double x1 = cos(dir1);
	double y1 = sin(dir1);
	double x2 = cos(dir2);
	double y2 = sin(dir2);
	double dot = x1 * x2 + y1 * y2;
	double angle = acos(dot);
	return angle;
}

INLINE__ bool RangeOverlap(float range1_min, float range1_max, float range2_min, float range2_max) {
	return range1_min < range2_max && range2_min < range1_max;
}

INLINE__ bool RangeOverlap(double range1_min, double range1_max, double range2_min, double range2_max) {
	return range1_min < range2_max && range2_min < range1_max;
}
