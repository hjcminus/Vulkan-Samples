/******************************************************************************
 model
 *****************************************************************************/

#pragma once

/*
================================================================================
VkModel
================================================================================
*/
class COMMON_API VkModel {
public:

	struct load_params_s {
		// use set 1
		VkDescriptorSetLayout	desc_set_layout_bind0_mat_bind1_tex_;
	};

	struct vk_material_s {
		vk_image_s			vk_texture_;
		VkDescriptorSet		vk_desc_set_;
	};

	struct vk_model_part_s {
		uint32_t			material_idx_;
		uint32_t			index_offset_;
		uint32_t			index_count_;
	};

	VkModel(VkDemo * owner);
	~VkModel();

	bool					Load(const load_params_s & params, const char * filename, bool move_to_origin);
	void					Free();

	vertex_format_t			GetVertexFormat() const;
	const float *			GetMin() const;
	const float *			GetMax() const;
	uint32_t				GetIndexCount() const;

	VkBuffer				GetVertexBuffer() const;
	VkBuffer				GetIndexBuffer() const;

	uint32_t				GetMaterialCount() const;
	const vk_material_s *	GetMaterialByIdx(uint32_t idx) const;

	uint32_t				GetModelPartCount() const;
	const vk_model_part_s *	GetModelPartByIdx(uint32_t idx) const;

private:

	VkDemo *				owner_;

	// sampler
	VkSampler				vk_sampler_;

	vk_buffer_s				vertex_buffer_;
	vk_buffer_s				index_buffer_;
	uint32_t				index_count_;

	vertex_format_t			vertex_format_;
	float					min_[3];
	float					max_[3];

	vk_buffer_s				vk_buffer_ubo_materials_;

	vk_material_s*			vk_materials_;
	vk_model_part_s*		parts_;	

	uint32_t				material_count_;
	uint32_t				part_count_;
};
