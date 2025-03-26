/******************************************************************************
 gen_terrain_mid_point.cpp
   
   using mid-point algorithm to genrate a random terrain mesh
 *****************************************************************************/

#include "inc.h"

bool gen_terrain_mid_point(terrain_size_t sz, float min_z, float max_z, float roughness, terrain_s& terrain) {
	// clear result
	terrain.vertex_count_per_edge_ = 0;
	terrain.heights_ = nullptr;

	int vertex_count_per_edge = Terrain_GetVertexCountPerEdge(sz);

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

	auto set_buffer_z = [&vertex_count_per_edge](float * buffer, int idx_x, int idx_y, float value) {
		buffer[idx_y * vertex_count_per_edge + idx_x] = value;
	};

	auto get_buffer_z = [&vertex_count_per_edge](float* buffer, int idx_x, int idx_y) {
		return buffer[idx_y * vertex_count_per_edge + idx_x];
	};

	// init delta value
	float delta = (max_z - min_z) * 0.5f;

	// calculate in range [-delta, delta]

	set_buffer_z(heights_buffer, 0, 0, RandNeg1Pos1() * delta);
	set_buffer_z(heights_buffer, vertex_count_per_edge - 1, 0, RandNeg1Pos1() * delta);
	set_buffer_z(heights_buffer, vertex_count_per_edge - 1, vertex_count_per_edge - 1, RandNeg1Pos1() * delta);
	set_buffer_z(heights_buffer, 0, vertex_count_per_edge - 1, RandNeg1Pos1() * delta);

	int step = vertex_count_per_edge - 1;
	while (step > 0) {

		// center pass
		for (int y = step / 2; y < (vertex_count_per_edge - 1); y += step) {
			for (int x = step / 2; x < (vertex_count_per_edge - 1); x += step) {

				float h0 = get_buffer_z(heights_buffer, x - step / 2, y - step / 2);
				float h1 = get_buffer_z(heights_buffer, x + step / 2, y - step / 2);
				float h2 = get_buffer_z(heights_buffer, x + step / 2, y + step / 2);
				float h3 = get_buffer_z(heights_buffer, x - step / 2, y + step / 2);

				float h = (h0 + h1 + h2 + h3) * 0.25f + (RandNeg1Pos1() >= 0.0f ? 1.0f : -1.0f) * delta;

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
				set_buffer_z(heights_buffer, x - step / 2, y, (z_ctr + z_lf_ct + z_lf_tp + z_lf_bt) * 0.25f + (RandNeg1Pos1() >= 0.0 ? 1.0f : -1.0f) * delta);

				// right edge
				set_buffer_z(heights_buffer, x + step / 2, y, (z_ctr + z_rt_ct + z_rt_tp + z_rt_bt) * 0.25f + (RandNeg1Pos1() >= 0.0 ? 1.0f : -1.0f) * delta);

				// top edge
				set_buffer_z(heights_buffer, x, y + step / 2, (z_ctr + z_tp_ct + z_lf_tp + z_rt_tp) * 0.25f + (RandNeg1Pos1() >= 0.0 ? 1.0f : -1.0f) * delta);

				// bottom edge
				set_buffer_z(heights_buffer, x, y - step / 2, (z_ctr + z_bt_ct + z_lf_bt + z_rt_bt) * 0.25f + (RandNeg1Pos1() >= 0.0 ? 1.0f : -1.0f) * delta);
			}
		}

		step /= 2;
		delta *= powf(2.0f, -1.0f * roughness);
	}

	// filter height values
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

	// shift to user defined range
	int total_vertex = vertex_count_per_edge * vertex_count_per_edge;
	float real_min_z = 9999999999.0f;
	float real_max_z = -9999999999.0f;
	for (int i = 0; i < total_vertex; ++i) {
		float z = heights_buffer[i];
		if (z < real_min_z) {
			real_min_z = z;
		}
		if (z > real_max_z) {
			real_max_z = z;
		}
	}

	float real_delta = real_max_z - real_min_z;
	float real_delta_inv = 1.0f / real_delta;
	float idea_delta = max_z - min_z;

	for (int i = 0; i < total_vertex; ++i) {
		float old_z = heights_buffer[i];
		
		float f = (old_z - real_min_z) * real_delta_inv;
		float new_z = min_z + idea_delta * f;

		// clamp
		if (new_z < min_z) {
			new_z = min_z;
		}

		if (new_z > max_z) {
			new_z = max_z;
		}

		heights_buffer[i] = new_z;
	}

	terrain.vertex_count_per_edge_ = vertex_count_per_edge;
	terrain.heights_ = heights_buffer;

	return true;
}
