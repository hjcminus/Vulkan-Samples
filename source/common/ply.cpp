/******************************************************************************
 load ply file
 *****************************************************************************/

#include "inc.h"

/*
================================================================================
PLY
================================================================================
*/
PLY::PLY() :
	buf_(nullptr),
	cur_read_pos_(nullptr),
	buf_end_(nullptr),
	format_(file_format_t::ASCII),
	num_property_(0),
	positions_(nullptr),
	normals_(nullptr),
	uv_lst_(nullptr),
	colors_(nullptr),
	num_vertex_(0),
	indices_(nullptr),
	num_index_(0),
	num_face_(0),
	num_triangles_(0),
	has_normal_(false),
	has_tex_coord_(false),
	has_color_(false)
{
	cur_token_[0] = '\0';
}

PLY::~PLY() {
	Clear();
}

bool PLY::Load(const char* filename) {
	Clear();

	int32_t file_size = 0;
	if (!File_LoadText(filename, buf_, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	cur_read_pos_ = buf_;
	buf_end_ = buf_ + file_size;
	cur_token_[0] = '\0';

	if (!LoadHead()) {
		return false;
	}

	if (!LoadData()) {
		return false;
	}

	if (!has_normal_) {
		CalculateNormals();
	}

	return true;
}

void PLY::Clear() {
	SAFE_FREE(indices_);
	SAFE_FREE(colors_);
	SAFE_FREE(uv_lst_);
	SAFE_FREE(normals_);
	SAFE_FREE(positions_);

	num_triangles_ = num_face_ = num_index_ = num_vertex_ = 0;

	num_property_ = 0;

	if (buf_) {
		File_FreeText(buf_);
		buf_ = nullptr;
	}

	cur_read_pos_ = buf_end_ = nullptr;
	cur_token_[0] = '\0';
}

const glm::vec3* PLY::GetPos() const {
	return positions_;
}

const glm::vec3* PLY::GetNormal() const {
	return normals_;
}

const glm::vec2* PLY::GetNV() const {
	return uv_lst_;
}

const glm::vec4* PLY::GetColor() const {
	return colors_;
}

uint32_t PLY::NumberVertex() const {
	return num_vertex_;
}

const uint32_t* PLY::GetIndices() const {
	return indices_;
}

uint32_t PLY::NumberTriangle() const {
	return num_triangles_;
}

bool PLY::Reader_IsValidChar(const char c) {
	return (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| (c == '.')
		|| (c == '_')
		|| (c == '-');
}

void PLY::Reader_SkipWhiteChar() {
	char c = *cur_read_pos_;
	while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
		cur_read_pos_++;
		c = *cur_read_pos_;
	}
}

bool PLY::Reader_Read(void* buf, int expect_readlen) {
	if (cur_read_pos_ + expect_readlen > buf_end_) {
		return false;
	}

	memcpy(buf, cur_read_pos_, expect_readlen);
	cur_read_pos_ += expect_readlen;

	return true;
}

const char* PLY::Reader_GetToken() {
	int token_len = 0;

	Reader_SkipWhiteChar();

	while (Reader_IsValidChar(*cur_read_pos_)) {
		if (token_len == 1024) {
			printf("token too long\n");
			return nullptr; // error
		}

		cur_token_[token_len++] = *cur_read_pos_;
		cur_read_pos_++;
	}

	cur_read_pos_++;
	cur_token_[token_len] = 0;

	return cur_token_;
}

bool PLY::Reader_MatchToken(const char* expect_token) {
	const char* token = Reader_GetToken();
	if (!token) {
		return false;
	}
	return strcmp(token, expect_token) == 0;
}

bool PLY::Reader_FindToken(const char* expect_token) {
	const char* token = Reader_GetToken();
	while (token) {
		if (!strcmp(token, expect_token)) {
			return true;
		}
		token = Reader_GetToken();
	}
	return false;
}

bool PLY::LoadHead() {
	if (!CheckHeadBegin()) {
		return false;
	}

	if (!LoadFormat()) {
		return false;
	}

	if (!LoadVertexCount()) {
		return false;
	}

	if (!LoadProperties()) {
		return false;
	}

	if (!LoadFaceCount()) {
		return false;
	}

	if (!CheckHeadEnd()) {
		return false;
	}

	return true;
}

bool PLY::LoadData() {
	if (num_vertex_ == 0 || num_face_ == 0) {
		printf("bad vertex number or face number\n");
		return false;
	}

	positions_ = (glm::vec3*)TEMP_ALLOC(sizeof(glm::vec3) * num_vertex_);

	if (has_color_) {
		colors_ = (glm::vec4*)TEMP_ALLOC(sizeof(glm::vec4) * num_vertex_);
	}

	normals_ = (glm::vec3*)TEMP_ALLOC(sizeof(glm::vec3) * num_vertex_);

	// we support up to 4 vertices per face, alloc max memory space.
	indices_ = (uint32_t*)TEMP_ALLOC(sizeof(uint32_t) * num_face_ * 2 * 3);

	if (format_ == file_format_t::ASCII) {
		return LoadData_ASCII();
	}
	else {
		return LoadData_BINARY();
	}
}

bool PLY::LoadData_ASCII() {
	// read vertices

	for (uint32_t i = 0; i < num_vertex_; ++i) {
		float x = 0.0f, y = 0.0f, z = 0.0f;
		float nx = 0.0f, ny = 0.0f, nz = 1.0f;
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f; // color if any

		for (uint32_t p = 0; p < num_property_; ++p) {

			const char* token = Reader_GetToken();
			if (!token) {
				return false;
			}

			float value = 0.0f;

			switch (properties_[p].type_) {
			case property_type_t::FLOAT:
				value = (float)atof(token);
				break;
			case property_type_t::UINT8:
				value = atoi(token) / 255.0f;
				break;
			default:
				printf("Unsupported data type: %d\n", properties_[p].type_);
				return false;
			}

			switch (properties_[p].name_) {
			case property_name_t::POS_X:
				x = value;
				break;
			case property_name_t::POS_Y:
				y = value;
				break;
			case property_name_t::POS_Z:
				z = value;
				break;
			case property_name_t::NORMAL_X:
				nx = value;
				break;
			case property_name_t::NORMAL_Y:
				ny = value;
				break;
			case property_name_t::NORMAL_Z:
				nz = value;
				break;
			case property_name_t::RED:
				r = value;
				break;
			case property_name_t::GREEN:
				g = value;
				break;
			case property_name_t::BLUE:
				b = value;
				break;
			case property_name_t::ALPHA:
				a = value;
				break;
			}
		}

		positions_[i].x = x;
		positions_[i].y = y;
		positions_[i].z = z;

		normals_[i].x = nx;
		normals_[i].y = ny;
		normals_[i].z = nz;

		if (colors_) {
			colors_[i][0] = r;
			colors_[i][1] = g;
			colors_[i][2] = b;
			colors_[i][3] = a;
		}
	}

	// read faces

	num_triangles_ = 0;

	for (uint32_t i = 0; i < num_face_; ++i) {

		const char* token = Reader_GetToken();
		if (!token) {
			break;
		}

		uint32_t num_vertex_of_face = (uint32_t)atoi(token);

		if (num_vertex_of_face == 3) {
			for (uint32_t j = 0; j < 3; ++j) {
				token = Reader_GetToken();
				if (!token) {
					return false;
				}

				uint32_t index = (uint32_t)atoi(token);

				if (index >= num_vertex_) {
					printf("invalid vertex index has been found\n");
					return false;
				}

				indices_[num_triangles_ * 3 + j] = index;
			}

			num_triangles_++;
		}
		else if (num_vertex_of_face == 4) {
			uint32_t indices_buf[4];

			for (uint32_t j = 0; j < 4; ++j) {
				token = Reader_GetToken();
				if (!token) {
					return false;
				}

				indices_buf[j] = (uint32_t)atoi(token);

				if (indices_buf[j] >= num_vertex_) {
					printf("invalid vertex index has been found\n");
					return false;
				}
			}

			indices_[num_triangles_ * 3] = indices_buf[0];
			indices_[num_triangles_ * 3 + 1] = indices_buf[1];
			indices_[num_triangles_ * 3 + 2] = indices_buf[2];
			indices_[num_triangles_ * 3 + 3] = indices_buf[2];
			indices_[num_triangles_ * 3 + 4] = indices_buf[3];
			indices_[num_triangles_ * 3 + 5] = indices_buf[0];

			num_triangles_ += 2;
		}
		else {
			printf("Unsupported face vertex count: %u\n", num_vertex_of_face);
			return false;
		}
	}

	return true;
}

bool PLY::LoadData_BINARY() {
	bool local_platform_is_big_endian = IsBigEndian();

	bool need_swap = (local_platform_is_big_endian && format_ == file_format_t::BINARY_LIT)
		|| (!local_platform_is_big_endian && format_ == file_format_t::BINARY_BIG);

	cur_read_pos_++;	// skip '\n'

	if (cur_read_pos_ >= buf_end_) {
		printf("reached end\n");
		return false;
	}

	// read vertices

	for (uint32_t i = 0; i < num_vertex_; ++i) {
		float x = 0.0f, y = 0.0f, z = 0.0f;
		float nx = 0.0f, ny = 0.0f, nz = 1.0f;
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f; // color if any

		for (uint32_t p = 0; p < num_property_; ++p) {
			float value = 0.0f;

			switch (properties_[p].type_) {
			case property_type_t::FLOAT:
			{
				unsigned char buffer1[4], buffer2[4];

				if (!Reader_Read(buffer1, 4)) {
					printf("read buffer failed\n");
					return false;
				}

				if (need_swap) {
					buffer2[0] = buffer1[3];
					buffer2[1] = buffer1[2];
					buffer2[2] = buffer1[1];
					buffer2[3] = buffer1[0];

					value = *(const float*)buffer2;
				}
				else {
					value = *(const float*)buffer1;
				}
			}
			break;
			case property_type_t::UINT8:
			{
				unsigned char buffer;

				if (!Reader_Read(&buffer, 1)) {
					printf("read buffer failed\n");
					return false;
				}

				value = buffer / 255.0f;
			}
			break;
			default:
				printf("Unsupported data type: %d\n", properties_[p].type_);
				return false;
			}

			switch (properties_[p].name_) {
			case property_name_t::POS_X:
				x = value;
				break;
			case property_name_t::POS_Y:
				y = value;
				break;
			case property_name_t::POS_Z:
				z = value;
				break;
			case property_name_t::NORMAL_X:
				nx = value;
				break;
			case property_name_t::NORMAL_Y:
				ny = value;
				break;
			case property_name_t::NORMAL_Z:
				nz = value;
				break;
			case property_name_t::RED:
				r = value;
				break;
			case property_name_t::GREEN:
				g = value;
				break;
			case property_name_t::BLUE:
				b = value;
				break;
			case property_name_t::ALPHA:
				a = value;
				break;
			}
		}

		positions_[i].x = x;
		positions_[i].y = y;
		positions_[i].z = z;

		normals_[i].x = nx;
		normals_[i].y = ny;
		normals_[i].z = nz;

		if (colors_) {
			colors_[i][0] = r;
			colors_[i][1] = g;
			colors_[i][2] = b;
			colors_[i][3] = a;
		}
	}

	// read faces

	num_triangles_ = 0;

	for (uint32_t i = 0; i < num_face_; ++i) {

		unsigned char num_vertex_of_face;

		if (!Reader_Read(&num_vertex_of_face, 1)) {
			printf("Read buffer failed\n");
			return false;
		}

		if (num_vertex_of_face == 3) {

			for (uint32_t j = 0; j < 3; ++j) {
				unsigned char buffer1[4], buffer2[4];

				if (!Reader_Read(buffer1, 4)) {
					printf("Read buffer failed\n");
					return false;
				}

				uint32_t index = 0;

				if (need_swap) {
					buffer2[0] = buffer1[3];
					buffer2[1] = buffer1[2];
					buffer2[2] = buffer1[1];
					buffer2[3] = buffer1[0];

					index = *(uint32_t*)buffer2;
				}
				else {
					index = *(uint32_t*)buffer1;
				}

				if (index >= num_vertex_) {
					printf("Invalid vertex index has been found\n");
					return false;
				}

				indices_[num_triangles_ * 3 + j] = index;
			}

			num_triangles_++;
		}
		else if (num_vertex_of_face == 4) {
			uint32_t indices_buf[4];

			for (uint32_t j = 0; j < 4; ++j) {
				unsigned char buffer1[4], buffer2[4];

				if (!Reader_Read(buffer1, 4)) {
					printf("Read buffer failed\n");
					return false;
				}

				uint32_t index = 0;

				if (need_swap) {
					buffer2[0] = buffer1[3];
					buffer2[1] = buffer1[2];
					buffer2[2] = buffer1[1];
					buffer2[3] = buffer1[0];

					index = *(uint32_t*)buffer2;
				}
				else {
					index = *(uint32_t*)buffer1;
				}

				if (index >= num_vertex_) {
					printf("Invalid vertex index has been found\n");
					return false;
				}

				indices_buf[j] = index;
			}

			indices_[num_triangles_ * 3] = indices_buf[0];
			indices_[num_triangles_ * 3 + 1] = indices_buf[1];
			indices_[num_triangles_ * 3 + 2] = indices_buf[2];
			indices_[num_triangles_ * 3 + 3] = indices_buf[2];
			indices_[num_triangles_ * 3 + 4] = indices_buf[3];
			indices_[num_triangles_ * 3 + 5] = indices_buf[0];

			num_triangles_ += 2;
		}
		else {
			printf("Unsupported face vertex count: %u\n", num_vertex_of_face);
			return false;
		}

	}

	return true;
}

void PLY::CalculateNormals() {
	// calculate normals
	memset(normals_, 0, sizeof(glm::vec3) * num_vertex_);

	for (uint32_t i = 0; i < num_triangles_; ++i) {
		int idxx = indices_[i * 3 + 0];
		int idxy = indices_[i * 3 + 1];
		int idxz = indices_[i * 3 + 2];

		glm::vec3 * v0 = positions_ + idxx;
		glm::vec3 * v1 = positions_ + idxy;
		glm::vec3 * v2 = positions_ + idxz;

		glm::vec3 v10 = *v1 - *v0;
		glm::vec3 v20 = *v2 - *v0;

		glm::vec3 normal = glm::cross(v10, v20);

		normals_[idxx] += normal;
		normals_[idxy] += normal;
		normals_[idxz] += normal;
	}

	for (uint32_t i = 0; i < num_vertex_; ++i) {
		glm::normalize(normals_[i]);
	}
}


bool PLY::CheckHeadBegin() {
	// check identification
	if (!Reader_MatchToken("ply")) {
		printf("Bad ply file\n");
		return false;
	}
	return true;
}

bool PLY::LoadFormat() {
	// load format
	if (!Reader_MatchToken("format")) {
		printf("No format section found\n");
		return false;
	}

	const char * token = Reader_GetToken();
	if (!token) {
		printf("Could not get format information\n");
		return false;
	}

	if (!strcmp(token, "ascii")) {
		format_ = file_format_t::ASCII;
	}
	else if (!strcmp(token, "binary_big_endian")) {
		format_ = file_format_t::BINARY_BIG;
	}
	else if (!strcmp(token, "binary_little_endian")) {
		format_ = file_format_t::BINARY_LIT;
	}
	else {
		printf("Unknown file format\n");
		return false;
	}

	return true;
}

bool PLY::LoadVertexCount() {
	// load vertex count
	if (!Reader_FindToken("element")) {
		printf("Could not find element section\n");
		return false;
	}

	if (!Reader_MatchToken("vertex")) {
		printf("Could not find vertex section\n");
		return false;
	}

	const char * token = Reader_GetToken();
	if (!token) {
		printf("Could not get token\n");
		return false;
	}

	num_vertex_ = (uint32_t)atoi(token);

	return true;
}

bool PLY::LoadProperties() {
	// load properties
	if (!Reader_FindToken("property")) {
		printf("Could not find property section\n");
		return false;
	}

	const char* token = nullptr;
	do {
		if (num_property_ == MAX_PROPERTY) {
			printf("Too many properties\n");
			return false;
		}

		property_s* prop = properties_ + num_property_++;

		// get property type
		token = Reader_GetToken();
		if (!token) {
			printf("Could not get token\n");
			return false;
		}

		if (!strcmp(token, "float32") || !strcmp(token, "float")) {
			prop->type_ = property_type_t::FLOAT;
		}
		else if (!strcmp(token, "uint8") || !strcmp(token, "uchar")) {
			prop->type_ = property_type_t::UINT8;
		}
		else {
			printf("Unsupported type: %s\n", token);
			return false;
		}

		// get property name
		token = Reader_GetToken();
		if (!token) {
			printf("Could not get token\n");
			return false;
		}

		if (!strcmp(token, "x")) {
			prop->name_ = property_name_t::POS_X;
		}
		else if (!strcmp(token, "y")) {
			prop->name_ = property_name_t::POS_Y;
		}
		else if (!strcmp(token, "z")) {
			prop->name_ = property_name_t::POS_Z;
		}
		else if (!strcmp(token, "nx")) {
			prop->name_ = property_name_t::NORMAL_X;
			has_normal_ = true;
		}
		else if (!strcmp(token, "ny")) {
			prop->name_ = property_name_t::NORMAL_Y;
			has_normal_ = true;
		}
		else if (!strcmp(token, "nz")) {
			prop->name_ = property_name_t::NORMAL_Z;
			has_normal_ = true;
		}
		else if (!strcmp(token, "s")) {
			prop->name_ = property_name_t::U;
			has_tex_coord_ = true;
		}
		else if (!strcmp(token, "t")) {
			prop->name_ = property_name_t::V;
			has_tex_coord_ = true;
		}
		else if (!strcmp(token, "confidence")) {
			prop->name_ = property_name_t::CONFIDENCE;
		}
		else if (!strcmp(token, "intensity")) {
			prop->name_ = property_name_t::INTENSITY;
		}
		else if (!strcmp(token, "red")) {
			prop->name_ = property_name_t::RED;
			has_color_ = true;
		}
		else if (!strcmp(token, "green")) {
			prop->name_ = property_name_t::GREEN;
			has_color_ = true;
		}
		else if (!strcmp(token, "blue")) {
			prop->name_ = property_name_t::BLUE;
			has_color_ = true;
		}
		else if (!strcmp(token, "alpha")) {
			prop->name_ = property_name_t::ALPHA;
			has_color_ = true;
		}
		
		else {
			printf("Unknown property name: %s\n", token);
		}

		// get next token
		token = Reader_GetToken();
		if (!token) {
			printf("Could not get token\n");
			return false;
		}
	} while (!strcmp(token, "property"));

	return true;
}

bool PLY::LoadFaceCount() {
	// get face count
	if (strcmp(cur_token_, "element")) {
		printf("Expect element section\n");
		return false;
	}

	if (!Reader_MatchToken("face")) {
		printf("Could not find face section\n");
		return false;
	}

	const char* token = Reader_GetToken();
	if (!token) {
		printf("Could not get token\n");
		return false;
	}

	num_face_ = (uint32_t)atoi(token);

	return true;
}

bool PLY::CheckHeadEnd() {
	if (!Reader_FindToken("end_header")) {
		printf("Could not find end_header section\n");
		return false;
	}
	return true;
}

