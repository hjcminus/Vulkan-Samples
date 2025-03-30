/******************************************************************************
 load obj model file
 *****************************************************************************/

#pragma once

/*
================================================================================
Obj
================================================================================
*/
class Obj {
public:

	static const int		NAME_LEN = 256;

	struct face_vertex_s {
		uint32_t			pos_idx_;
		uint32_t			normal_idx_;
		uint32_t			uv_idx_;
	};

	struct face_s {
		uint32_t			vertex_count_;	// 3 or 4
		face_vertex_s		vertices_[4];
	};

	struct face_group_s {
		char				name_[NAME_LEN];
		uint32_t			material_idx_;
		uint32_t			face_count_;
		face_s *			faces_;
	};

	// https://people.sc.fsu.edu/~jburkardt/data/mtl/mtl.html
	// material definition
	struct material_s {
		char				name_[NAME_LEN];
		char				map_Kd_[NAME_LEN];	// diffuse map
		char				map_N_[NAME_LEN];	// normal map
		char				map_Ks_[NAME_LEN];	// shiness map
		glm::vec3			Ka_;				// ambient color. default: (0.2,0.2,0.2)
		glm::vec3			Kd_;				// diffuse color. default: (0.8,0.8,0.8)
		glm::vec3			Ks_;				// specular color. default:  (1.0,1.0,1.0)
		float				shiness_;			// 1 ~ 1000
		float				alpha_;				// transparency
	};

	Obj();
	~Obj();

	bool					Load(const char* filename);
	void					Clear();

	const glm::vec3*		GetPos() const;
	uint32_t				NumberPos() const;

	const glm::vec3*		GetNormal() const;
	uint32_t				NumberNormal() const;

	const glm::vec2*		GetUV() const;
	uint32_t				NumberUV() const;

	const material_s *		GetMaterial() const;
	uint32_t				NumberMaterial() const;

	const face_group_s *	GetFaceGroup() const;
	uint32_t				NumFaceGroup() const;

private:

	glm::vec3 *				positions_;
	uint32_t				position_count_;

	glm::vec3 *				normals_;
	uint32_t				normal_count_;

	glm::vec2 *				uv_list_;
	uint32_t				uv_count_;

	face_s *				faces_;
	uint32_t				face_count_;

	face_group_s *			face_groups_;
	uint32_t				face_group_count_;

	material_s *			materials_;
	uint32_t				material_count_;

	bool					LoadMTL(const char * filename);
	bool					ParseFace(const char * line, uint32_t read_vertex_count, 
								uint32_t read_uv_count, uint32_t read_normal_count, face_s * face);

	uint32_t				GetMaterialIdxByName(const char * mat_name) const;
	void					CalcNormals();
};
