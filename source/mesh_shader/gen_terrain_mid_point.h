/******************************************************************************
 gen_terrain_mid_point.h

 using mid-point algorithm to genrate a random terrain mesh
 *****************************************************************************/

#pragma once

enum class terrain_size_t : int {	// edge length of the squared terrain
	TS_32,
	TS_64,
	TS_128,
	TS_256,
	TS_512,
	TS_1K
};

struct terrain_s {
	int			vertex_count_per_edge_;
	float *		heights_;
};

bool gen_terrain_mid_point(terrain_size_t sz, float min_z, float max_z, terrain_s & terrain);
void free_terrain(terrain_s& terrain);
