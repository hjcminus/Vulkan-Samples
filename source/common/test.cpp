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

void test_frustum() {
	const float left = -1.0f;
	const float right = 1.0f;
	const float top = 1.0f;
	const float bottom = -1.0f;
	const float z_near = 1.0f;
	const float z_far = 3.0f;


	glm::mat4 proj_RH_NO = glm::frustumRH_NO(left, right, bottom, top, z_near, z_far);
	glm::mat4 proj_RH_ZO = glm::frustumRH_ZO(left, right, bottom, top, z_near, z_far);
	glm::mat4 proj_LH_ZO = glm::frustumLH_ZO(left, right, bottom, top, z_near, z_far);

	// 8 points of a cube 
	std::array<glm::vec4, 8> points = {
		// bottom
		glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), 
		glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f), 
		glm::vec4( 1.0f, -1.0f, -3.0f, 1.0f), 
		glm::vec4(-1.0f, -1.0f, -3.0f, 1.0f),
		// top
		glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f), 
		glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f, -3.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f, -3.0f, 1.0f),
	};

	printf("-- glm::frustumRH_NO --\n");
	for (auto& it : points) {
		glm::vec4 pt = proj_RH_NO * it;
		printf("<%9.6f, %9.6f, %9.6f, %9.6f> -> <%9.6f, %9.6f, %9.6f, %9.6f>\n",
			it.x, it.y, it.z, it.w, pt.x, pt.y, pt.z, pt.w);
	}

/* output:

<-1.000000, -1.000000, -1.000000,  1.000000> -> <-1.000000, -1.000000, -1.000000,  1.000000>
< 1.000000, -1.000000, -1.000000,  1.000000> -> < 1.000000, -1.000000, -1.000000,  1.000000>
< 1.000000, -1.000000, -3.000000,  1.000000> -> < 1.000000, -1.000000,  3.000000,  3.000000>
<-1.000000, -1.000000, -3.000000,  1.000000> -> <-1.000000, -1.000000,  3.000000,  3.000000>

<-1.000000,  1.000000, -1.000000,  1.000000> -> <-1.000000,  1.000000, -1.000000,  1.000000>
< 1.000000,  1.000000, -1.000000,  1.000000> -> < 1.000000,  1.000000, -1.000000,  1.000000>
< 1.000000,  1.000000, -3.000000,  1.000000> -> < 1.000000,  1.000000,  3.000000,  3.000000>
<-1.000000,  1.000000, -3.000000,  1.000000> -> <-1.000000,  1.000000,  3.000000,  3.000000>

*/

	printf("-- glm::frustumRH_ZO --\n");
	for (auto& it : points) {
		glm::vec4 pt = proj_RH_ZO * it;
		printf("<%9.6f, %9.6f, %9.6f, %9.6f> -> <%9.6f, %9.6f, %9.6f, %9.6f>\n",
			it.x, it.y, it.z, it.w, pt.x, pt.y, pt.z, pt.w);
	}

/* output:
	
<-1.000000, -1.000000, -1.000000,  1.000000> -> <-1.000000, -1.000000,  0.000000,  1.000000>
< 1.000000, -1.000000, -1.000000,  1.000000> -> < 1.000000, -1.000000,  0.000000,  1.000000>
< 1.000000, -1.000000, -3.000000,  1.000000> -> < 1.000000, -1.000000,  3.000000,  3.000000>
<-1.000000, -1.000000, -3.000000,  1.000000> -> <-1.000000, -1.000000,  3.000000,  3.000000>

<-1.000000,  1.000000, -1.000000,  1.000000> -> <-1.000000,  1.000000,  0.000000,  1.000000>
< 1.000000,  1.000000, -1.000000,  1.000000> -> < 1.000000,  1.000000,  0.000000,  1.000000>
< 1.000000,  1.000000, -3.000000,  1.000000> -> < 1.000000,  1.000000,  3.000000,  3.000000>
<-1.000000,  1.000000, -3.000000,  1.000000> -> <-1.000000,  1.000000,  3.000000,  3.000000>
	
*/

	// revert y
	printf("-- glm::frustumRH_ZO (revert y) --\n");
	proj_RH_ZO[1][1] *= -1.0f;
	for (auto& it : points) {
		glm::vec4 pt = proj_RH_ZO * it;
		printf("<%9.6f, %9.6f, %9.6f, %9.6f> -> <%9.6f, %9.6f, %9.6f, %9.6f>\n",
			it.x, it.y, it.z, it.w, pt.x, pt.y, pt.z, pt.w);
	}

/* output:

<-1.000000, -1.000000, -1.000000,  1.000000> -> <-1.000000,  1.000000,  0.000000,  1.000000>
< 1.000000, -1.000000, -1.000000,  1.000000> -> < 1.000000,  1.000000,  0.000000,  1.000000>
< 1.000000, -1.000000, -3.000000,  1.000000> -> < 1.000000,  1.000000,  3.000000,  3.000000>
<-1.000000, -1.000000, -3.000000,  1.000000> -> <-1.000000,  1.000000,  3.000000,  3.000000>
<-1.000000,  1.000000, -1.000000,  1.000000> -> <-1.000000, -1.000000,  0.000000,  1.000000>
< 1.000000,  1.000000, -1.000000,  1.000000> -> < 1.000000, -1.000000,  0.000000,  1.000000>
< 1.000000,  1.000000, -3.000000,  1.000000> -> < 1.000000, -1.000000,  3.000000,  3.000000>
<-1.000000,  1.000000, -3.000000,  1.000000> -> <-1.000000, -1.000000,  3.000000,  3.000000>

*/


/*
	printf("-- glm::frustumLH_ZO --\n");
	for (auto& it : points) {
		glm::vec4 pt = proj_LH_ZO * it;
		printf("<%9.6f, %9.6f, %9.6f, %9.6f> -> <%9.6f, %9.6f, %9.6f, %9.6f>\n",
			it.x, it.y, it.z, it.w, pt.x, pt.y, pt.z, pt.w);
	}
*/


/* output:

<-1.000000, -1.000000, -1.000000,  1.000000> -> <-1.000000, -1.000000, -3.000000, -1.000000>
< 1.000000, -1.000000, -1.000000,  1.000000> -> < 1.000000, -1.000000, -3.000000, -1.000000>
< 1.000000, -1.000000, -3.000000,  1.000000> -> < 1.000000, -1.000000, -6.000000, -3.000000>
<-1.000000, -1.000000, -3.000000,  1.000000> -> <-1.000000, -1.000000, -6.000000, -3.000000>

<-1.000000,  1.000000, -1.000000,  1.000000> -> <-1.000000,  1.000000, -3.000000, -1.000000>
< 1.000000,  1.000000, -1.000000,  1.000000> -> < 1.000000,  1.000000, -3.000000, -1.000000>
< 1.000000,  1.000000, -3.000000,  1.000000> -> < 1.000000,  1.000000, -6.000000, -3.000000>
<-1.000000,  1.000000, -3.000000,  1.000000> -> <-1.000000,  1.000000, -6.000000, -3.000000>

*/

}

static void test() {
/*
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::mat3 m;
	m[0].x = right.x;
	m[1].x = right.y;
	m[2].x = right.z;

	m[0].y = up.x;
	m[1].y = up.y;
	m[2].y = up.z;

	m[0].z = -forward.x;
	m[1].z = -forward.y;
	m[2].z = -forward.z;

	glm::vec3 v = glm::vec3(0.0f, 0.0f, -1.0f);

	glm::vec3 v2 = m * v;

	printf("(%f,%f,%f) -> (%f,%f,%f)\n", v.x, v.y, v.z, v2.x, v2.y, v2.z);

	glm::mat3 m2 = glm::transpose(m);

	glm::vec3 v3 = m2 * v2;
	printf("(%f,%f,%f) -> (%f,%f,%f)\n", v2.x, v2.y, v2.z, v3.x, v3.y, v3.z);
*/

	frustum_s frustum = {
		.z_near_ = 1.0f,
		.z_far_ = 3.0f,
		.fovy_ = 90.0f,
		.ratio_ = 1.0f,
		.view_pos_ = glm::vec3(0.0f, -2.0f, 0.0f),
		.view_target_ = glm::vec3(0.0f),
		.view_up_ = glm::vec3(0.0f, 0.0f, 1.0f)
	};

	glm::vec3 corners[8];

	CalculateFrustumCorners(frustum, corners);

	for (int i = 0; i < 8; ++i) {
		printf("[%d] %f,%f,%f\n", i, corners[i].x, corners[i].y, corners[i].z);
	}
}

void test_dds(const char * filename) {
	printf("--test_dds: %s--\n", filename);

	char full_filename[MAX_PATH];
	Str_SPrintf(full_filename, COUNT_OF(full_filename), "%s/textures/%s", 
		GetDataFolder(), filename);

	image_s image = {};
	if (Img_Load(full_filename, image)) {
		Str_ReplaceFileNameExt(full_filename, COUNT_OF(full_filename), "png");
		Img_Save(full_filename, image);
		Img_Free(image);
	}
}

static void test_float16() {
	for (int i = 0; i < 100; ++i) {
		float f1 = Rand01();
		float16_t h = FloatToHalfFloat(f1);
		float f2 = HalfFloatToFloat(h);
		printf("[%3d] f1 = %f, f2 = %f\n", i, f1, f2);
	}
}

int main(int argc, char** argv) {
	Common_Init();

	//test_proj_mat(1.0f, 16.0f);
	//test_frustum();

	//test();

	//test_float16();

	//test_dds("ocean/perlin_noise.dds");
	test_dds("ocean/sky_cube.dds");
	//test_dds("ocean/reflect_cube.dds");

	return 0;
}

#endif
