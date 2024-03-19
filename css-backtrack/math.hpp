#pragma once
#include "vector.hpp"
#include <cmath>

#define M_PI 3.14159265359f
#define MD_PI 3.14159265359

constexpr auto deg_per_rad = 57.2957795131f;

float rad_to_deg(float x) {
	return x * deg_per_rad;
}

void vector_angles(vec3_t& forward, vec3_t& angles) {
	if (forward.x == 0.f && forward.y == 0.f) {
		angles.x = (forward.z > 0.f) ? 270.f : 90.f;
		angles.y = 0.f;
	}
	else {
		angles.x = -rad_to_deg(atan2(-forward.z, forward.length_2d()));
		angles.y = rad_to_deg(atan2(forward.y, forward.x));

		if (angles.y > 90.f)
			angles.y -= 180.f;
		else if (angles.y < 90.f)
			angles.y += 180.f;
		else if (angles.y == 90.f)
			angles.y = 0.f;
	}

	angles.z = 0.f;
}

union ieee754_float {
	uint32_t full;

	struct
	{
		uint32_t fraction : 23;
		uint32_t exponent : 8;
		uint32_t sign : 1;
	};
};

vec3_t calc_angle(const vec3_t& from, const vec3_t& to) {
	vec3_t angles;
	vec3_t delta = from - to;

	vector_angles(delta, angles);

	angles.clamp();

	return angles;
}

float abs_val(float value) {
	auto copy_val = value;
	reinterpret_cast<ieee754_float*>(&copy_val)->sign = 0;

	return copy_val;
}

float calc_fov(const vec3_t& from, const vec3_t& to, const vec3_t& view_angle) {
	auto ideal_angle = calc_angle(from, to);

	ideal_angle -= view_angle;

	if (ideal_angle.x > 180.f)
		ideal_angle.x -= 360.f;
	else if (ideal_angle.x < -180.f)
		ideal_angle.x += 360.f;

	if (ideal_angle.y > 180.f)
		ideal_angle.y -= 360.f;
	else if (ideal_angle.y < -180.f)
		ideal_angle.y += 360.f;

	ideal_angle.x = abs_val(ideal_angle.x);
	ideal_angle.y = abs_val(ideal_angle.y);

	return ideal_angle.x + ideal_angle.y;
}

struct mat3x4 {
	float c[3][4];
};