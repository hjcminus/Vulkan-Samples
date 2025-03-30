/******************************************************************************
 Angle
 *****************************************************************************/

/*
================================================================================
Angle
================================================================================
*/

template<class REAL>
INLINE__ void Angle<REAL>::FromVector(const Vec3<REAL>& dir) {
	angles_[ANGLE_ROLL] = (REAL)0.0;

	if (std::fabs(dir.y_) == 0.0 && std::fabs(dir.x_) == 0.0) {
		angles_[ANGLE_YAW] = (REAL)0.0;
	}
	else {
		REAL r = (REAL)std::atan2(dir.y_, dir.x_);

		REAL d = (REAL)RadToDeg(r);
		d -= (REAL)90.0;

		if (d < 0.0) {
			d += (REAL)360.0;
		}

		angles_[ANGLE_YAW] = d;
	}

	REAL forward_proj = std::sqrt(dir.x_ * dir.x_ + dir.y_ * dir.y_);
	REAL r2 = (REAL)std::atan2(dir.z_, forward_proj);
	REAL d2 = (REAL)RadToDeg(r2);

	if (d2 > (REAL)90.0) {
		d2 = (REAL)90.0;
	}

	if (d2 < (REAL)-90.0) {
		d2 = (REAL)-90.0;
	}

	angles_[ANGLE_PITCH] = d2;
}

template<class REAL>
INLINE__ void Angle<REAL>::ToVectors(Vec3<REAL>* vec_forward, Vec3<REAL>* vec_right, Vec3<REAL>* vec_up) const {
	REAL yaw   = DegToRad(angles_[ANGLE_YAW]);
	REAL pitch = DegToRad(angles_[ANGLE_PITCH]);
	REAL roll  = DegToRad(angles_[ANGLE_ROLL]);

	REAL cos_y = (REAL)std::cos(yaw);
	REAL sin_y = (REAL)std::sin(yaw);

	REAL cos_p = (REAL)std::cos(pitch);
	REAL sin_p = (REAL)std::sin(pitch);

	REAL cos_r = (REAL)std::cos(roll);
	REAL sin_r = (REAL)std::sin(roll);

	if (vec_forward) {
		vec_forward->x_ = -sin_y * cos_p;
		vec_forward->y_ =  cos_y * cos_p;
		vec_forward->z_ =  sin_p;
	}

	if (vec_right) {
		vec_right->x_   =  cos_y * cos_r - sin_p * sin_y * sin_r;
		vec_right->y_   =  sin_y * cos_r + sin_p * cos_y * sin_r;
		vec_right->z_   = -cos_p * sin_r;
	}

	if (vec_up) {
		vec_up->x_ = sin_y * sin_p * cos_r + cos_y * sin_r;
		vec_up->y_ = sin_y * sin_r - cos_y * sin_p * cos_r;
		vec_up->z_ = cos_p * cos_r;
	}
}
