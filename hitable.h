#pragma once

#ifndef HITABLEH
#define HITABLEH

#include "ray.h"

class material;

typedef struct hit_record {
	float t;
	vec3 p;
	vec3 normal;
	material *mat_ptr;
} hit_record;

class hitable {
public:
	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
};

class hitable_list : public hitable {
public:
	hitable_list() {}
	hitable_list(hitable **l, int n) {
		list = l;
		list_size = n;
	}
	virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;

	hitable **list;
	int list_size;
};

#endif