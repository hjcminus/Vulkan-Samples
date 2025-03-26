/******************************************************************************
 test common library
 *****************************************************************************/

#include "../common/inc.h"

int main(int argc, char** argv) {
	Common_Init();

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
		return 1;
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

	return 0;
}
