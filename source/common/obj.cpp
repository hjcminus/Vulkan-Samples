/******************************************************************************
 load obj file
 *****************************************************************************/

#include "inc.h"

/*
================================================================================
parser
================================================================================
*/
static void Line_ParseString(const char* line, char s[Obj::NAME_LEN]) {
	char first_part[Obj::NAME_LEN];
	*s = 0;
	char* parts[2] = { first_part, s };

	Str_Tokenize(line, parts, Obj::NAME_LEN, 2);
};

static void Line_ParseVec1(const char* s, float& v) {
	v = 0.0f;

	char str_h[64] = {};	// head
	char str_x[64] = {};

	char* parts[2] = { str_h, str_x };

	int tokens = Str_Tokenize(s, parts, 64, 2);

	if (tokens == 2) {
		v = (float)atof(str_x);
	}
}

static void Line_ParseVec2(const char* s, glm::vec2& v) {
	v.x = 0.0f;
	v.y = 0.0f;

	char str_h[64] = {};	// head
	char str_x[64] = {};
	char str_y[64] = {};

	char* parts[3] = { str_h, str_x, str_y };

	int tokens = Str_Tokenize(s, parts, 64, 3);

	if (tokens == 3) {
		v.x = (float)atof(str_x);
		v.y = (float)atof(str_y);
	}
}

static void Line_ParseVec3(const char* s, glm::vec3& v) {
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 0.0f;

	char str_h[64] = {};	// head
	char str_x[64] = {};
	char str_y[64] = {};
	char str_z[64] = {};

	char* parts[4] = { str_h, str_x, str_y, str_z };

	int tokens = Str_Tokenize(s, parts, 64, 4);

	if (tokens == 4) {
		v.x = (float)atof(str_x);
		v.y = (float)atof(str_y);
		v.z = (float)atof(str_z);
	}
}

/*
================================================================================
Obj
================================================================================
*/
Obj::Obj():
	positions_(nullptr),
	position_count_(0),
	normals_(nullptr),
	normal_count_(0),
	uv_list_(nullptr),
	uv_count_(0),
	faces_(nullptr),
	face_count_(0),
	face_groups_(nullptr),
	face_group_count_(0),
	materials_(nullptr),
	material_count_(0)
{
	//
}

Obj::~Obj() {
	Clear();
}

bool Obj::Load(const char* filename) {
	Clear();

	char* text = nullptr;
	int32_t file_size = 0;
	if (!File_LoadText(filename, text, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	Lexer lexer;
	lexer.SetPtr(text);

	// counting
	const char* line = nullptr;
	while (line = lexer.ReadLine()) {
		if (strncmp(line, "mtllib ", 7) == 0) {
			char mat_filename[256];
			Line_ParseString(line, mat_filename);

			char filename_folder[MAX_PATH] = {};
			Str_ExtractFileDir(filename, filename_folder, MAX_PATH);

			char full_mat_filename[MAX_PATH];
			Str_SPrintf(full_mat_filename, MAX_PATH, "%s/%s", filename_folder, mat_filename);

			if (!LoadMTL(full_mat_filename)) {
				printf("Load mtl file failed\n");
			}
		}
		else if (strncmp(line, "v ", 2) == 0) {
			position_count_++;
		}
		else if (strncmp(line, "vt ", 3) == 0) {
			uv_count_++;
		}
		else if (strncmp(line, "vn ", 3) == 0) {
			normal_count_++;
		}
		else if (strncmp(line, "usemtl ", 7) == 0) {
			face_group_count_++;
		}
		else if (strncmp(line, "f ", 2) == 0) {
			face_count_++;
		}
	}

	bool default_face_group = false;

	if (!face_group_count_) {
		face_group_count_++;	// create a default one
		default_face_group = true;
	}

	if (!position_count_ || !face_count_ || !face_group_count_) {
		File_FreeText(text);
		printf("Bad model\n");
		return false;
	}
	else {
		// allocate memory
		positions_ = (glm::vec3*)TEMP_ALLOC(sizeof(glm::vec3) * position_count_);
		if (!positions_) {
			return false;
		}
		
		if (normal_count_) {
			normals_ = (glm::vec3*)TEMP_ALLOC(sizeof(glm::vec3) * normal_count_);
			if (!normals_) {
				return false;
			}
		}

		if (uv_count_) {
			uv_list_ = (glm::vec2*)TEMP_ALLOC(sizeof(glm::vec2) * uv_count_);
			if (!uv_list_) {
				return false;
			}
		}

		faces_ = (face_s*)TEMP_ALLOC(sizeof(face_s) * face_count_);
		if (!faces_) {
			return false;
		}

		face_groups_ = (face_group_s*)TEMP_ALLOC(sizeof(face_group_s) * face_group_count_);
		if (!face_groups_) {
			return false;
		}

		// read data
		uint32_t read_position_idx = 0;
		uint32_t read_normal_idx = 0;
		uint32_t read_uv_idx = 0;

		uint32_t parsed_face_count = 0;
		uint32_t parsed_face_group_count = 0;

		face_group_s* cur_face_group = nullptr;
		if (default_face_group) {
			cur_face_group = face_groups_;

			cur_face_group->material_idx_ = (uint32_t)-1;
			cur_face_group->faces_ = faces_ + parsed_face_count;
			cur_face_group->face_count_ = 0;
		}

		void* t = this;

		int read_lines = 0;
		lexer.SetPtr(text);
		while (line = lexer.ReadLine()) {
			read_lines++;

			if (this != t) {
				int bk = 0;
			}

			if (strncmp(line, "v ", 2) == 0) {
				Line_ParseVec3(line, positions_[read_position_idx++]);
			}
			else if (strncmp(line, "vn ", 3) == 0) {
				if (normals_) {
					Line_ParseVec3(line, normals_[read_normal_idx++]);
				}
			}
			else if (strncmp(line, "vt ", 3) == 0) {
				if (uv_list_) {
					Line_ParseVec2(line, uv_list_[read_uv_idx++]);
				}
			}
			/*else if (strncmp(line, "o ", 2) == 0) {
				char obj_name[NAME_LEN];
				Line_ParseString(line, obj_name);

				cur_face_group = face_groups_ + parsed_face_group_count++;
				Str_Copy(cur_face_group->name_, NAME_LEN, obj_name);
				cur_face_group->material_idx_ = VK_INVALID_INDEX;
				cur_face_group->face_count_ = 0;
				cur_face_group->faces_ = faces_ + parsed_face_count;
			}
			*/
			else if (strncmp(line, "usemtl ", 7) == 0) {
				char mat_name[NAME_LEN];
				Line_ParseString(line, mat_name);

				cur_face_group = face_groups_ + parsed_face_group_count++;
				Str_Copy(cur_face_group->name_, NAME_LEN, mat_name);
				cur_face_group->material_idx_ = VK_INVALID_INDEX;
				cur_face_group->face_count_ = 0;
				cur_face_group->faces_ = faces_ + parsed_face_count;


				uint32_t mat_idx = GetMaterialIdxByName(mat_name);
				if (mat_idx == VK_INVALID_INDEX) {
					printf("Material %s not found\n", mat_name);
				}

				if (parsed_face_group_count) {
					cur_face_group = &face_groups_[parsed_face_group_count - 1];
					cur_face_group->material_idx_ = mat_idx;
				}
				else {
					cur_face_group = face_groups_ + parsed_face_group_count++;
					Str_Copy(cur_face_group->name_, NAME_LEN, mat_name);	// set as mat name
					cur_face_group->material_idx_ = mat_idx;
					cur_face_group->face_count_ = 0;
					cur_face_group->faces_ = faces_ + parsed_face_count;
				}
			}
			else if (strncmp(line, "f ", 2) == 0) {
				if (!cur_face_group) {
					continue; // ignore this face
				}

				if (!ParseFace(line, read_position_idx, read_uv_idx, read_normal_idx,
					faces_ + parsed_face_count++)) {
					File_FreeText(text);
					printf("Bad face\n");
					return false;
				}

				cur_face_group->face_count_++;
			}
		}

		File_FreeText(text);

		if (!normals_) {
			CalcNormals();
		}

		return true;
	}
}

void Obj::Clear() {
	SAFE_FREE(materials_);
	SAFE_FREE(face_groups_);
	SAFE_FREE(faces_);
	SAFE_FREE(uv_list_);
	SAFE_FREE(normals_);
	SAFE_FREE(positions_);

	position_count_ = 0;
	normal_count_ = 0;
	uv_count_ = 0;
	face_count_ = 0;
	face_group_count_ = 0;
	material_count_ = 0;
}

const glm::vec3* Obj::GetPos() const {
	return positions_;
}

uint32_t Obj::NumberPos() const {
	return position_count_;
}

const glm::vec3* Obj::GetNormal() const {
	return normals_;
}

uint32_t Obj::NumberNormal() const {
	return normal_count_;
}

const glm::vec2* Obj::GetUV() const {
	return uv_list_;
}

uint32_t Obj::NumberUV() const {
	return uv_count_;
}

const Obj::material_s* Obj::GetMaterial() const {
	return materials_;
}

uint32_t Obj::NumberMaterial() const {
	return material_count_;
}

const Obj::face_group_s* Obj::GetFaceGroup() const {
	return face_groups_;
}

uint32_t Obj::NumFaceGroup() const {
	return face_group_count_;
}

bool Obj::LoadMTL(const char* filename) {
	char* text = nullptr;
	int32_t file_size = 0;
	if (!File_LoadText(filename, text, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	Lexer lexer;
	lexer.SetPtr(text);

	material_count_ = 0;
	const char* line = nullptr;
	while (line = lexer.ReadLine()) {
		if (strncmp(line, "newmtl ", 7) == 0) {
			material_count_++;
		}
	}

	if (!material_count_) {
		File_FreeText(text);
		printf("No material definition\n");
		return false;
	}

	materials_ = (material_s*)TEMP_ALLOC(sizeof(material_s) * material_count_);

	// read data
	uint32_t parsed_material_count = 0;

	material_s* cur_material = nullptr;
	bool skip_cur_material = false;

	lexer.SetPtr(text);	// reset to beginning
	
	while (line = lexer.ReadLine()) {
		if (strncmp(line, "newmtl ", 7) == 0) {

			char s[256] = {};
			Line_ParseString(line, s);

			cur_material = materials_ + parsed_material_count;
			memset(cur_material, 0, sizeof(*cur_material));

			Str_Copy(cur_material->name_, NAME_LEN, s);
			cur_material->Ka_ = glm::vec3(0.2f);
			cur_material->Kd_ = glm::vec3(0.8f);
			cur_material->Ks_ = glm::vec3(1.0f);
			cur_material->shiness_ = 0.0f;
			cur_material->alpha_ = 1.0f;

			skip_cur_material = false;

			for (uint32_t i = 0; i < parsed_material_count; ++i) {
				if (strcmp(materials_[i].name_, cur_material->name_) == 0) {
					skip_cur_material = true; // duplicated material
					break;
				}
			}

			if (!skip_cur_material) {
				parsed_material_count++;
			}

		}
		else if (strncmp(line, "Ns ", 3) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				Line_ParseVec1(line, cur_material->shiness_);
			}
		}
		else if (strncmp(line, "Ka ", 3) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				Line_ParseVec3(line, cur_material->Ka_);
			}
		}
		else if (strncmp(line, "Kd ", 3) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				Line_ParseVec3(line, cur_material->Kd_);
			}
		}
		else if (strncmp(line, "Ks ", 3) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				Line_ParseVec3(line, cur_material->Ks_);
			}
		}
		else if (strncmp(line, "d ", 2) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				Line_ParseVec1(line, cur_material->alpha_);
			}
		}
		else if (strncmp(line, "map_Kd ", 7) == 0) {
			if (!skip_cur_material) {
				if (!cur_material) {
					File_FreeText(text);
					printf("Bad mtl format\n");
					return false;
				}

				char s[256] = {};
				Line_ParseString(line, s);

				Str_ExtractFileName(s, cur_material->map_Kd_, NAME_LEN);
			}
		}
	}

	// override material_count_
	material_count_ = parsed_material_count;

	File_FreeText(text);

	return true;
}

bool Obj::ParseFace(const char* line, uint32_t read_position_count,
	uint32_t read_uv_count, uint32_t read_normal_count, face_s* face)
{
	char head[NAME_LEN];
	char part1[NAME_LEN];
	char part2[NAME_LEN];
	char part3[NAME_LEN];
	char part4[NAME_LEN];

	char* parts[5] = { head, part1, part2, part3, part4 };

	int tokens = Str_Tokenize(line, parts, NAME_LEN, 5);

	int vertex_count = tokens - 1;
	if (vertex_count != 3 && vertex_count != 4) {
		return false;
	}

	face->vertex_count_ = vertex_count;

	uint32_t read_counts[3] = { read_position_count, read_uv_count, read_normal_count };

	for (int i = 0; i < vertex_count; ++i) {
		char* str_indices[3] = {};

		// vertex index/uv index/normal index
		int item_count = Str_Split(parts[1 + i], '/', str_indices, 3);

		uint32_t indices[3];
		memset(indices, 0xff, sizeof(indices));

		for (int j = 0; j < item_count; ++j) {
			int idx = atoi(str_indices[j]);

			if (idx < 0) {
				// negative indices: e.g. -1 references the last vertex defined
				int adjust_idx = (int)read_counts[j] + idx + 1;
				if (adjust_idx <= 0 || (uint32_t)adjust_idx > read_counts[j]) {
					return false;	// out of range
				}
				indices[j] = (uint32_t)adjust_idx;
			}
			else {
				indices[j] = (uint32_t)idx;	// positive
			}
		}

		// -1 convert to [0~num-1]
		face->vertices_[i].pos_idx_ = indices[0] - 1;
		face->vertices_[i].uv_idx_ = indices[1] - 1;
		face->vertices_[i].normal_idx_ = indices[2] - 1;
	}

	// fill rest to invalid index
	for (uint32_t i = face->vertex_count_; i < 4; ++i) {
		face->vertices_[i].pos_idx_ = VK_INVALID_INDEX;
		face->vertices_[i].normal_idx_ = VK_INVALID_INDEX;
		face->vertices_[i].uv_idx_ = VK_INVALID_INDEX;
	}

	return true;
}

uint32_t Obj::GetMaterialIdxByName(const char* mat_name) const {
	for (uint32_t i = 0; i < material_count_; ++i) {
		if (!strcmp(mat_name, materials_[i].name_)) {
			return i;
		}
	}
	return VK_INVALID_INDEX;
}

void Obj::CalcNormals() {
	if (normals_) {
		return;
	}

	/*

	 2
	  |\
	  |  \
	  |    \
	  |      \
	  0------- 1

	  accumulate normal at vertex 0

	*/
	auto AccumulateNormal = [](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& norm) {
		glm::vec3 edge10 = v1 - v0;
		glm::vec3 edge20 = v2 - v0;
		glm::vec3 n = glm::cross(edge10, edge20);
		norm += n;
	};

	normal_count_ = position_count_;
	normals_ = (glm::vec3*)TEMP_ALLOC(sizeof(glm::vec3) * normal_count_);
	memset(normals_, 0, sizeof(glm::vec3) * normal_count_); // clear to 0,0,0

	glm::vec3 * norm = nullptr;
	for (uint32_t i = 0; i < face_group_count_; ++i) {
		face_group_s * fg = face_groups_ + i;
		for (uint32_t j = 0; j < fg->face_count_; ++j) {
			face_s * f = fg->faces_ + j;

			if (f->vertex_count_ == 3) {
				norm = normals_ + f->vertices_[0].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[0].pos_idx_],
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[2].pos_idx_], *norm);
				f->vertices_[0].normal_idx_ = f->vertices_[0].pos_idx_;

				norm = normals_ + f->vertices_[1].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[2].pos_idx_],
					positions_[f->vertices_[0].pos_idx_], *norm);
				f->vertices_[1].normal_idx_ = f->vertices_[1].pos_idx_;

				norm = normals_ + f->vertices_[2].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[2].pos_idx_],
					positions_[f->vertices_[0].pos_idx_],
					positions_[f->vertices_[1].pos_idx_], *norm);
				f->vertices_[2].normal_idx_ = f->vertices_[2].pos_idx_;
			}
			else {
				// f->vertex_count_ == 4
				norm = normals_ + f->vertices_[0].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[0].pos_idx_],
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[3].pos_idx_], *norm);
				f->vertices_[0].normal_idx_ = f->vertices_[0].pos_idx_;

				norm = normals_ + f->vertices_[1].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[3].pos_idx_],
					positions_[f->vertices_[0].pos_idx_], *norm);
				AccumulateNormal(
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[2].pos_idx_],
					positions_[f->vertices_[3].pos_idx_], *norm);
				f->vertices_[1].normal_idx_ = f->vertices_[1].pos_idx_;

				norm = normals_ + f->vertices_[2].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[2].pos_idx_],
					positions_[f->vertices_[3].pos_idx_],
					positions_[f->vertices_[1].pos_idx_], *norm);
				f->vertices_[2].normal_idx_ = f->vertices_[2].pos_idx_;

				norm = normals_ + f->vertices_[3].pos_idx_;
				AccumulateNormal(
					positions_[f->vertices_[3].pos_idx_],
					positions_[f->vertices_[1].pos_idx_],
					positions_[f->vertices_[2].pos_idx_], *norm);
				AccumulateNormal(
					positions_[f->vertices_[3].pos_idx_],
					positions_[f->vertices_[0].pos_idx_],
					positions_[f->vertices_[1].pos_idx_], *norm);
				f->vertices_[3].normal_idx_ = f->vertices_[3].pos_idx_;
			}
		}
	}

	for (uint32_t i = 0; i < normal_count_; ++i) {
		normals_[i] = glm::normalize(normals_[i]);
	}
}
