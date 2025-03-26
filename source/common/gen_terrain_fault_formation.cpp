/******************************************************************************
 gen_terrain_fault_formation.cpp

 using fault formation algorithm to genrate a random terrain mesh
 *****************************************************************************/

#include "inc.h"

bool gen_terrain_fault_formation(terrain_size_t sz, float min_z, float max_z, 
	int iterations, float filter, terrain_s& terrain) {

	/*
	        y
	        |  edge 2
	        |__________
	        |          |
	edge 3  |          | edge 1
	        |__________|_________x
	           edge 0

	*/

	auto GetRandomVec2 = [](uint32_t vertex_count_per_edge, int edge) -> glm::vec2 {
		float dim = (float)(vertex_count_per_edge - 1);

		switch (edge) {
		case 0:
			return glm::vec2(dim * Rand01(), 0.0f);
		case 1:
			return glm::vec2(dim, dim * Rand01());
		case 2:
			return glm::vec2(dim * Rand01(), dim);
		default:	// case 3
			return glm::vec2(0.0f, dim * Rand01());
		}
	};

	auto UpdateZ = [](float* heights_buffer, uint32_t vertex_count_per_edge, glm::vec2 pt0, glm::vec2 pt1, float z_add) {
		glm::vec2 pt1_pt0 = pt1 - pt0;
		
		for (uint32_t y = 0; y < vertex_count_per_edge; ++y) {
			for (uint32_t x = 0; x < vertex_count_per_edge; ++x) {
				glm::vec2 pt = glm::vec2(x, y);

				glm::vec2 pt_pt0 = pt - pt0;

				glm::vec3 c = glm::cross(glm::vec3(pt_pt0, 0.0f), glm::vec3(pt1_pt0, 0.0f));
				if (c.z > 0.0f) {
					// on right side of line pt0 -> pt1
					heights_buffer[y * vertex_count_per_edge + x] += z_add;
				}
			}
		}
	};

	// filter: [0 ~ 1]
	auto Erode = [](float* heights_buffer, uint32_t vertex_count_per_edge, float filter) {

		// FIR filter
		auto FilterHeightBand = [](float* height_band, uint32_t vertex_count, uint32_t step, uint32_t count, float filter) {
			float v = height_band[0];
			uint32_t j = step;

			// go through the height band and apply the erosion filter
			for (uint32_t i = 0; i < count - 1; ++i) {
				if (j >= vertex_count) {
					break;
				}

				height_band[j] = filter * v + (1 - filter) * height_band[j];

				v = height_band[j];
				j += step;
			}
		};

		uint32_t vertex_count = vertex_count_per_edge * vertex_count_per_edge;

		// erode left to right

		/*
		
		y
		|____
		|    |
		0____|__x

		  -->
		
		*/

		for (uint32_t i = 0; i < vertex_count_per_edge; ++i) {
			FilterHeightBand(heights_buffer + i, vertex_count,
				vertex_count_per_edge /*step*/, vertex_count_per_edge, filter);
		}

		// erode right to left

		/*

		y
		|____
		|    |
		0____|__x

		  <--

		*/

		for (uint32_t i = 0; i < vertex_count_per_edge; ++i) {
			FilterHeightBand(heights_buffer + vertex_count_per_edge - i - 1, vertex_count,
				vertex_count_per_edge /*step*/, vertex_count_per_edge, filter);
		}

		// erode top to bottom

		/*

		y            |
		|____        |
		|    |      \|/
		0____|__x

		*/

		for (uint32_t i = 0; i < vertex_count_per_edge; ++i) {
			FilterHeightBand(
				heights_buffer + vertex_count_per_edge * (vertex_count_per_edge - 1) - i * vertex_count_per_edge,
				vertex_count,
				1 /*step*/, vertex_count_per_edge, filter);
		}

		// erode from bottom to top

		/*

		y           /|\
		|____        |
		|    |       |
		0____|__x    |

		*/

		for (uint32_t i = 0; i < vertex_count_per_edge; ++i) {
			FilterHeightBand(
				heights_buffer + i * vertex_count_per_edge,
				vertex_count,
				1 /*step*/, vertex_count_per_edge, filter);
		}
	};

	// clear result
	terrain.vertex_count_per_edge_ = 0;
	terrain.heights_ = nullptr;

	uint32_t vertex_count_per_edge = Terrain_GetVertexCountPerEdge(sz);

	// allocate buffers
	float* heights_buffer = new float[vertex_count_per_edge * vertex_count_per_edge];
	if (!heights_buffer) {
		printf("Could not alloc height buffer\n");
		return false;
	}

	// fill zero
	memset(heights_buffer, 0, sizeof(float) * vertex_count_per_edge * vertex_count_per_edge);

	for (int i = 0; i < iterations; ++i) {

		float add_z = max_z - ((max_z - min_z) * i / iterations);

		int edge0 = Rand() % 4;
		int edge1 = (edge0 + (Rand() % 3) + 1) % 4;

		glm::vec2 pt0 = GetRandomVec2(vertex_count_per_edge, edge0);
		glm::vec2 pt1 = GetRandomVec2(vertex_count_per_edge, edge1);

		UpdateZ(heights_buffer, vertex_count_per_edge, pt0, pt1, add_z);
	}

	Erode(heights_buffer, vertex_count_per_edge, 0.33f);

	terrain.vertex_count_per_edge_ = vertex_count_per_edge;
	terrain.heights_ = heights_buffer;

	return true;
}
