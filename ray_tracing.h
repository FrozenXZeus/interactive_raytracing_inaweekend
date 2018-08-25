#pragma once

#include "hitable.h"

hitable *random_scene();
void ray_trace(int width, int height, int height_start, int height_end, hitable *world, unsigned char *buffer);
