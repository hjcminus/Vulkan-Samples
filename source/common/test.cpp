/******************************************************************************
 test common library
 *****************************************************************************/

#include "../common/inc.h"

#ifndef COMMON_MODULE

void gen_terrain_tex() {
	//*
	terrain_gen_params_s terrain_gen_params = {
		.algo_ = terrain_gen_algorithm_t::FAULT_FORMATION,
		.sz_ = terrain_size_t::TS_128,
		.min_z_ = 0.0f,
		.max_z_ = 2.5f,
		.iterations_ = 128,
		.filter_ = 0.4f,	// 0.15 ~ 0.75
		.roughness_ = 0.0f	// not used
	};
	//*/

	/*
	terrain_gen_params_s terrain_gen_params = {
		.algo_ = terrain_gen_algorithm_t::MID_POINT,
		.sz_ = terrain_size_t::TS_512,
		.min_z_ = 0.0f,
		.max_z_ = 64.0f,
		.iterations_ = 0,	// not used
		.filter_ = 0.0f,		// not used
		.roughness_ = 0.75f,	// 0.25 ~ 1.5
	};
	//*/
	terrain_s terrain = {};

	if (!Terrain_Generate(terrain_gen_params, terrain)) {
		return;
	}

	const char* data_foler = GetDataFolder();

	terrain_texture_tiles_s tiles;

	Str_SPrintf(tiles.lowest_, MAX_PATH, "%s/textures/terrain/lowestTile.tga", data_foler);
	Str_SPrintf(tiles.low_, MAX_PATH, "%s/textures/terrain/lowTile.tga", data_foler);
	Str_SPrintf(tiles.high_, MAX_PATH, "%s/textures/terrain/HighTile.tga", data_foler);
	Str_SPrintf(tiles.hightest_, MAX_PATH, "%s/textures/terrain/highestTile.tga", data_foler);

	image_s texture = {};

	char save_texture[MAX_PATH];
	Str_SPrintf(save_texture, MAX_PATH, "%s/textures/terrain/combined.bmp", data_foler);

	if (Terrain_Texture(tiles, terrain, 512, 512, texture)) {
		Img_Save(save_texture, texture);
		Img_Free(texture);
	}

	Terrain_Free(terrain);
}

void test_proj_mat(float z_near, float z_far) {
	auto PrintMat = [z_near,z_far](const char* name, const glm::mat4& m) {
		glm::vec4 v_near = glm::vec4(0.0f, 0.0f, -z_near, 1.0f);
		glm::vec4 v_far = glm::vec4(0.0f, 0.0f, -z_far, 1.0f);

		printf("---------- %s ----------\n", name);
		for (int i = 0; i < 4; ++i) {
			printf("[%f,%f,%f,%f]\n", m[0][i], m[1][i], m[2][i], m[3][i]);
		}

		glm::vec4 v_near_clip = m * v_near;
		glm::vec4 v_far_clip = m * v_far;
		printf("v_near_clip = <%f,%f,%f,%f>\n", v_near_clip.x, v_near_clip.y, v_near_clip.z, v_near_clip.w);
		printf("v_far_clip = <%f,%f,%f,%f>\n", v_far_clip.x, v_far_clip.y, v_far_clip.z, v_far_clip.w);
	};

	//PrintMat("glm::perspective", glm::perspective(glm::radians(45.0f),
	//	640.0f / 480.0f, z_near, z_far));

	PrintMat("glm::perspectiveRH_NO", glm::perspectiveRH_NO(glm::radians(45.0f),
		640.0f / 480.0f, z_near, z_far));
	PrintMat("glm::perspectiveRH_ZO", glm::perspectiveRH_ZO(glm::radians(45.0f),
		640.0f / 480.0f, z_near, z_far));
	PrintMat("glm::perspectiveLH_NO", glm::perspectiveLH_NO(glm::radians(45.0f),
		640.0f / 480.0f, z_near, z_far));
	PrintMat("glm::perspectiveLH_ZO", glm::perspectiveLH_ZO(glm::radians(45.0f),
		640.0f / 480.0f, z_near, z_far));
}

int main(int argc, char** argv) {
	Common_Init();

	test_proj_mat(1.0f, 16.0f);

	return 0;
}

#endif
