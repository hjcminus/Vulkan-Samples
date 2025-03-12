/******************************************************************************
 gen_terrain_mid_point.cpp
   
   using mid-point algorithm to genrate a random terrain mesh
 *****************************************************************************/

#include "gen_terrain_mid_point.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <memory.h>

// return: [-1, 1]
static float randf() {
	float f = rand() / (float)RAND_MAX;
	return f * 2.0f - 1.0f;
}

bool gen_terrain_mid_point(terrain_size_t sz, float min_z, float max_z, terrain_s& terrain) {
	// clear result
	terrain.vertex_count_per_edge_ = 0;
	terrain.heights_ = nullptr;

	// determine the vertex count per edge of the squared terrain
	int vertex_count_per_edge = 513;
	switch (sz) {
	case terrain_size_t::TS_32:
		vertex_count_per_edge = 33;
		break;
	case terrain_size_t::TS_64:
		vertex_count_per_edge = 65;
		break;
	case terrain_size_t::TS_128:
		vertex_count_per_edge = 129;
		break;
	case terrain_size_t::TS_256:
		vertex_count_per_edge = 257;
		break;
	case terrain_size_t::TS_512:
		vertex_count_per_edge = 513;
		break;
	case terrain_size_t::TS_1K:
		vertex_count_per_edge = 1025;
		break;
	default:
		printf("unknown size %d, set vertex_count_per_edge to %d\n", sz, vertex_count_per_edge);
		break;
	}

	// allocate buffers
	float* heights_buffer = new float[vertex_count_per_edge * vertex_count_per_edge];
	if (!heights_buffer) {
		printf("Could not alloc height buffer\n");
		return false;
	}

	float* filter_buffer = new float[vertex_count_per_edge * vertex_count_per_edge];
	if (!filter_buffer) {
		delete [] heights_buffer;
		printf("Could not alloc filter_buffer buffer\n");
		return false;
	}

	// randomize
	srand((unsigned)time(NULL));

	auto set_buffer_z = [&vertex_count_per_edge](float * buffer, int idx_x, int idx_y, float value) {
		buffer[idx_y * vertex_count_per_edge + idx_x] = value;
	};

	auto get_buffer_z = [&vertex_count_per_edge](float* buffer, int idx_x, int idx_y) {
		return buffer[idx_y * vertex_count_per_edge + idx_x];
	};

	// init delta value
	float delta = (max_z - min_z) * 0.5f;

	// first: calculate in range [-delta, delta]

	set_buffer_z(heights_buffer, 0, 0, randf() * delta);
	set_buffer_z(heights_buffer, vertex_count_per_edge - 1, 0, randf() * delta);
	set_buffer_z(heights_buffer, vertex_count_per_edge - 1, vertex_count_per_edge - 1, randf() * delta);
	set_buffer_z(heights_buffer, 0, vertex_count_per_edge - 1, randf() * delta);

	int step = vertex_count_per_edge - 1;
	while (step > 0) {

		// center pass
		for (int y = step / 2; y < (vertex_count_per_edge - 1); y += step) {
			for (int x = step / 2; x < (vertex_count_per_edge - 1); x += step) {

				float h0 = get_buffer_z(heights_buffer, x - step / 2, y - step / 2);
				float h1 = get_buffer_z(heights_buffer, x + step / 2, y - step / 2);
				float h2 = get_buffer_z(heights_buffer, x + step / 2, y + step / 2);
				float h3 = get_buffer_z(heights_buffer, x - step / 2, y + step / 2);

				float h = (h0 + h1 + h2 + h3) * 0.25f + (randf() >= 0.0f ? 1.0f : -1.0f) * delta;

				set_buffer_z(heights_buffer, x, y, h);
			}
		}

		// edge pass
		for (int y = step / 2; y < (vertex_count_per_edge - 1); y += step) {
			for (int x = step / 2; x < (vertex_count_per_edge - 1); x += step) {

				float z_ctr   = get_buffer_z(heights_buffer, x, y);
				float z_lf_tp = get_buffer_z(heights_buffer, x - step / 2, y + step / 2);
				float z_lf_bt = get_buffer_z(heights_buffer, x - step / 2, y - step / 2);
				float z_rt_bt = get_buffer_z(heights_buffer, x + step / 2, y - step / 2);
				float z_rt_tp = get_buffer_z(heights_buffer, x + step / 2, y + step / 2);

				float z_lf_ct = (x - step >= 0) ? get_buffer_z(heights_buffer, x - step, y) : z_ctr;
				float z_rt_ct = (x + step < vertex_count_per_edge) ? get_buffer_z(heights_buffer, x + step, y) : z_ctr;
				float z_tp_ct = (y + step < vertex_count_per_edge) ? get_buffer_z(heights_buffer, x, y + step) : z_ctr;
				float z_bt_ct = (y - step >= 0) ? get_buffer_z(heights_buffer, x, y - step) : z_ctr;

				// left edge
				set_buffer_z(heights_buffer, x - step / 2, y, (z_ctr + z_lf_ct + z_lf_tp + z_lf_bt) * 0.25f + (randf() >= 0.0 ? 1.0f : -1.0f) * delta);

				// right edge
				set_buffer_z(heights_buffer, x + step / 2, y, (z_ctr + z_rt_ct + z_rt_tp + z_rt_bt) * 0.25f + (randf() >= 0.0 ? 1.0f : -1.0f) * delta);

				// top edge
				set_buffer_z(heights_buffer, x, y + step / 2, (z_ctr + z_tp_ct + z_lf_tp + z_rt_tp) * 0.25f + (randf() >= 0.0 ? 1.0f : -1.0f) * delta);

				// bottom edge
				set_buffer_z(heights_buffer, x, y - step / 2, (z_ctr + z_bt_ct + z_lf_bt + z_rt_bt) * 0.25f + (randf() >= 0.0 ? 1.0f : -1.0f) * delta);
			}
		}

		step /= 2;
		delta *= powf(2.0f, -3.0f);
	}

	// second: shift to user defined range
	int total_vertex = vertex_count_per_edge * vertex_count_per_edge;
	float shift_z = (min_z + max_z) * 0.5f;
	for (int i = 0; i < total_vertex; ++i) {
		heights_buffer[i] += shift_z;
	}

	// third: filter height values
	int filter_count = 4;	// tune this

	for (int f = 0; f < filter_count; ++f) {
		memcpy(filter_buffer, heights_buffer, sizeof(float) * vertex_count_per_edge* vertex_count_per_edge);
		for (int y = 0; y < vertex_count_per_edge; ++y) {
			for (int x = 0; x < vertex_count_per_edge; ++x) {

				float h0 = get_buffer_z(filter_buffer, x, y);
				float h1 = (x - 1 < 0) ? get_buffer_z(filter_buffer, x, y) : get_buffer_z(filter_buffer, x - 1, y);
				float h2 = (x + 1 >= vertex_count_per_edge) ? get_buffer_z(filter_buffer, x, y) : get_buffer_z(filter_buffer, x + 1, y);
				float h3 = (y - 1 < 0) ? get_buffer_z(filter_buffer, x, y) : get_buffer_z(filter_buffer, x, y - 1);
				float h4 = (y + 1 >= vertex_count_per_edge) ? get_buffer_z(filter_buffer, x, y) : get_buffer_z(filter_buffer, x, y + 1);

				int x_ = (x - 1 < 0) ? x : x - 1;
				int y_ = (y - 1 < 0) ? y : y - 1;
				float h5 = get_buffer_z(filter_buffer, x_, y_);

				x_ = (x - 1 < 0) ? x : x - 1;
				y_ = (y + 1 >= vertex_count_per_edge) ? y : y + 1;
				float h6 = get_buffer_z(filter_buffer, x_, y_);

				x_ = (x + 1 >= vertex_count_per_edge) ? x : x + 1;
				y_ = (y + 1 >= vertex_count_per_edge) ? y : y + 1;
				float h7 = get_buffer_z(filter_buffer, x_, y_);

				x_ = (x + 1 >= vertex_count_per_edge) ? x : x + 1;
				y_ = (y - 1 < 0) ? y : y - 1;
				float h8 = get_buffer_z(filter_buffer, x_, y_);

				set_buffer_z(heights_buffer, x, y, (h0 + h1 + h2 + h3 + h4 + h5 + h6 + h7 + h8) / 9.0f);
			}
		}
		// printf("filter: %d pass\n", f + 1);
	}

	delete [] filter_buffer;

	// clamp heights
	for (int i = 0; i < total_vertex; ++i) {
		if (heights_buffer[i] < min_z) {
			heights_buffer[i] = min_z;
		}
		if (heights_buffer[i] > max_z) {
			heights_buffer[i] = max_z;
		}
	}

	terrain.vertex_count_per_edge_ = vertex_count_per_edge;
	terrain.heights_ = heights_buffer;

	return true;
}

void free_terrain(terrain_s& terrain) {
	if (terrain.heights_) {
		delete [] terrain.heights_;
		terrain.heights_ = nullptr;
	}
}
