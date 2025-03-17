/******************************************************************************
 load ply file
 *****************************************************************************/

#pragma once

/*
================================================================================
PLY
================================================================================
*/
class PLY {
public:

	PLY();
	~PLY();

	bool				Load(const char* filename);
	void				Clear();

	const glm::vec3*	GetPos() const;
	const glm::vec3*	GetNormal() const;
	const glm::vec2*	GetNV() const;
	const glm::vec4*	GetColor() const;
	uint32_t			NumberVertex() const;

	const uint32_t*		GetIndices() const;
	uint32_t			NumberTriangle() const;

private:

	enum class file_format_t {
		ASCII,
		BINARY_BIG,
		BINARY_LIT
	};

	enum class property_type_t {
		FLOAT,
		UINT8
	};

	enum class property_name_t {
		POS_X,
		POS_Y,
		POS_Z,
		NORMAL_X,
		NORMAL_Y,
		NORMAL_Z,
		U,
		V,
		CONFIDENCE,
		INTENSITY,
		RED,
		GREEN,
		BLUE,
		ALPHA
	};

	static const int	MAX_PROPERTY = 32;

	struct property_s {
		property_type_t	type_;
		property_name_t	name_;
	};

	// buffer
	static const int	MAX_TOKEN_LEN = 1024;

	char*				buf_;
	char*				cur_read_pos_;
	char*				buf_end_;
	char				cur_token_[MAX_TOKEN_LEN];


	// in-memory information
	file_format_t		format_;

	property_s			properties_[MAX_PROPERTY];
	uint32_t			num_property_;

	glm::vec3*			positions_;
	glm::vec3*			normals_;
	glm::vec2*			uv_lst_;
	glm::vec4*			colors_;
	uint32_t			num_vertex_;

	uint32_t*			indices_;
	uint32_t			num_index_;
	uint32_t			num_face_;	// trangle or quad
	uint32_t			num_triangles_;

	bool				has_normal_;
	bool				has_tex_coord_;
	bool				has_color_;

	bool				Reader_IsValidChar(const char c);
	void				Reader_SkipWhiteChar();
	bool				Reader_Read(void* buf, int expect_readlen);
	const char*			Reader_GetToken();
	bool				Reader_MatchToken(const char * expect_token);
	bool				Reader_FindToken(const char* expect_token);


	bool				LoadHead();
	bool				LoadData();
	bool				LoadData_ASCII();
	bool				LoadData_BINARY();
	void				CalculateNormals();

	bool				CheckHeadBegin();
	bool				LoadFormat();
	bool				LoadVertexCount();
	bool				LoadProperties();
	bool				LoadFaceCount();
	bool				CheckHeadEnd();

};
