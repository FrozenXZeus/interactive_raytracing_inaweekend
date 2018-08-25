#include "ray_tracing.h"

#include <float.h>

#include "camera.h"
#include "ray.h"
#include "vec3.h"
#include "sphere.h"
#include "hitable.h"
#include "rng.h"

vec3 color(const ray& r, hitable *world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.0001f, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			return attenuation * color(scattered, world, depth + 1);
		}
		else {
			return vec3(0, 0, 0);
		}
	}
	else {
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);
		return (1.0f - t)*vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
	}
}

hitable *random_scene() {
	int n = 500;
	hitable **list = new hitable*[n + 1];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(0.5f, 0.5f, 0.5f)));
	int i = 1;
	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float choose_mat = drand48();
			vec3 center(a + 0.9f * drand48(), 0.2f, b + 0.9f*drand48());
			if ((center - vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.4f) {
					list[i++] = new sphere(center, 0.2f, new lambertian(vec3(drand48()*drand48(), drand48()*drand48(), drand48()*drand48())));
				}
				else if (choose_mat < 0.8f) {
					list[i++] = new sphere(center, 0.2f,
						new metal(vec3(0.5f*(1 + drand48()), 0.5f*(1 + drand48()), 0.5f*(1 + drand48())), 0.5f*drand48()));
				}
				else {
					list[i++] = new sphere(center, 0.2f, new dieletric(1.5f));
				}
			}
		}
	}

	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dieletric(1.5f));
	list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(vec3(0.4f, 0.2f, 0.1f)));
	list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7f, 0.6f, 0.5f), 0.0f));

	return new hitable_list(list, i);
}


void ray_trace(int width, int height, int height_start, int height_end, hitable *world, unsigned char *buffer) {
	vec3 lower_left_corner(-2.0f, -1.5f, -1.0f);
	vec3 horizontal(4.0f, 0.0f, 0.0f);
	vec3 vertical(0.0f, 3.0f, 0.0f);
	vec3 origin(0.0f, 0.0f, 0.0f);

	vec3 lookfrom(13, 2, 3);
	vec3 lookat(0, 0, 0);
	float dist_to_focus = 10.0f;
	float aperture = 0.1f;

	camera cam(lookfrom, lookat, vec3(0, 1, 0), 20, float(width) / float(height), aperture, dist_to_focus);

	int samples = 100;
	int current_pixel = height_start * width;
	buffer += 3 * current_pixel;
	for (int j = height_start; j < height_end; j++) {
		for (int i = 0; i < width; i++) {

			vec3 col(0, 0, 0);
			for (int s = 0; s < samples; s++) {
				float u = float(i + drand48()) / float(width);
				float v = float(j + drand48()) / float(height);

				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0);

				col += color(r, world, 0);
			}

			col /= float(samples);
			col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

			int ir = int(255.99f*col[0]);
			int ig = int(255.99f*col[1]);
			int ib = int(255.99f*col[2]);

			buffer[0] = unsigned char(ir);
			buffer[1] = unsigned char(ig);
			buffer[2] = unsigned char(ib);

			buffer += 3;
		}
	}
}