#pragma once
#include <immintrin.h>
#include <limits>

class vec3_t
{
public:
	vec3_t() = default;
	vec3_t(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	//~vec3_t();

	void init();

	vec3_t(const float* clr)
	{
		x = clr[0];
		y = clr[1];
		z = clr[2];
	}

	float x, y, z;

	vec3_t& operator+=(const vec3_t& v)
	{
		x += v.x; y += v.y; z += v.z; return *this;
	}
	vec3_t& operator-=(const vec3_t& v)
	{
		x -= v.x; y -= v.y; z -= v.z; return *this;
	}
	vec3_t& operator*=(float v)
	{
		x *= v; y *= v; z *= v; return *this;
	}
	vec3_t operator+(const vec3_t& v)
	{
		return vec3_t{ x + v.x, y + v.y, z + v.z };
	}
	const vec3_t operator+(const vec3_t& v) const
	{
		return vec3_t{ x + v.x, y + v.y, z + v.z };
	}
	vec3_t operator-(const vec3_t& v)
	{
		return vec3_t{ x - v.x, y - v.y, z - v.z };
	}
	const vec3_t operator-(const vec3_t& v) const
	{
		return vec3_t{ x - v.x, y - v.y, z - v.z };
	}
	vec3_t operator*(float v) const
	{
		return vec3_t{ x * v, y * v, z * v };
	}
	vec3_t operator/(float v) const
	{
		return vec3_t{ x / v, y / v, z / v };
	}
	float& operator[](int i)
	{
		return ((float*)this)[i];
	}
	float operator[](int i) const
	{
		return ((float*)this)[i];
	}

	void clamp() {
		while (this->z < -89.f)
			this->z += 89.f;

		if (this->z > 89.f)
			this->z = 89.f;

		while (this->y < -180.f)
			this->y += 360.f;

		while (this->y > 180.f)
			this->y -= 360.f;

		this->z = 0.f;
	}

	void normalize() {
		auto floorf = [&](float x) {
			return float((int)x);
			};

		auto roundf = [&](float x) {
			float t;

			//if (!isfinite(x))
			//	return x;

			if (x >= 0.0) {
				t = floorf(x);
				if (t - x <= -0.5)
					t += 1.0;
				return t;
			}
			else {
				t = floorf(-x);
				if (t + x <= -0.5)
					t += 1.0;
				return -t;
			}
			};

		auto absf = [](float x) {
			return x > 0.f ? x : -x;
			};

		auto x_rev = this->x / 360.f;
		if (this->x > 180.f || this->x < -180.f) {
			x_rev = absf(x_rev);
			x_rev = roundf(x_rev);

			if (this->x < 0.f)
				this->x = (this->x + 360.f * x_rev);

			else
				this->x = (this->x - 360.f * x_rev);
		}

		auto y_rev = this->y / 360.f;
		if (this->y > 180.f || this->y < -180.f) {
			y_rev = absf(y_rev);
			y_rev = roundf(y_rev);

			if (this->y < 0.f)
				this->y = (this->y + 360.f * y_rev);

			else
				this->y = (this->y - 360.f * y_rev);
		}

		auto z_rev = this->z / 360.f;
		if (this->z > 180.f || this->z < -180.f) {
			z_rev = absf(z_rev);
			z_rev = roundf(z_rev);

			if (this->z < 0.f)
				this->z = (this->z + 360.f * z_rev);

			else
				this->z = (this->z - 360.f * z_rev);
		}
	}
	vec3_t normalized() {
		vec3_t vec(*this);
		vec.normalize();

		return vec;
	}
	float normalize_in_place() {
		const float flLength = this->length();
		const float flRadius = 1.0f / (flLength + std::numeric_limits<float>::epsilon());

		this->x *= flRadius;
		this->y *= flRadius;
		this->z *= flRadius;

		return flLength;
	}
	float length() {
		const auto x_ = _mm_set_ss(this->x);
		const auto y_ = _mm_set_ss(this->y);
		const auto z_ = _mm_set_ss(this->z);

		return _mm_cvtss_f32(_mm_sqrt_ps(_mm_add_ss(_mm_mul_ss(z_, z_), _mm_add_ss(_mm_mul_ss(x_, x_), _mm_mul_ss(y_, y_)))));
	}

	float absolute_length() {
		auto fabs = [&](float a) {
			return a > 0 ? a : -a;
			};

		return fabs(x) + fabs(y) + fabs(z);
	}

	float length_2d() const
	{
		const auto x_ = _mm_set_ss(this->x);
		const auto y_ = _mm_set_ss(this->y);

		return _mm_cvtss_f32(_mm_sqrt_ps(_mm_add_ss(_mm_mul_ss(x_, x_), _mm_mul_ss(y_, y_))));
	}

	float length_sqr() {
		return (x * x + y * y + z * z);
	}
	float dot(const vec3_t other) { return (x * other.x + y * other.y + z * other.z); }
	float dist_to(vec3_t vec) const { return (vec - *this).length_2d(); } // lol
	float distance_2d(const vec3_t& a) { vec3_t delta = (*this - a); return delta.length_2d(); }
};

using vec3 = vec3_t;
