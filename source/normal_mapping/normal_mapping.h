/******************************************************************************
 normal mapping / bump mapping
 *****************************************************************************/

#pragma once

#include "../common/inc.h"

/*
================================================================================
NormalMappingDemo
================================================================================
*/
class NormalMappingDemo : public VkDemo {
public:

	NormalMappingDemo();
	~NormalMappingDemo() override;

	bool					Init();
	void					Shutdown();
	void					Display() override;

private:

	enum class render_mode_t {
		RM_TEXTURE,
		RM_FLAT_LIGHT,
		RM_NORMAL_MAPPING
	};

	struct vertex_s {
		glm::vec3			pos_;
		glm::vec3			normal_;
		glm::vec2			uv_;
		glm::vec3			tangent_;
	};

	struct texture_s {
		VkImage				image_;
		VkDeviceMemory		memory_;
		VkImageView			image_view_;
	};

	struct ubo_mat_s {
		glm::mat4			mat_proj_;
		glm::mat4			mat_view_;
		glm::mat4			mat_model_;
	};

	// 16 bytes aligned
	struct ubo_light_s {
		glm::vec4			light_pos_;
		glm::vec4			light_color_;
	};

	// sampler
	VkSampler				vk_sampler_;

	texture_s				texture_color_;
	texture_s				texture_normal_;

	buffer_s				uniform_buffer_mat_;
	buffer_s				uniform_buffer_light_;
	buffer_s				vertex_buffer_;
	buffer_s				index_buffer_;
	uint32_t				index_count_;

	// descriptor
	VkDescriptorSetLayout   vk_descriptorset_layout_uniform_;
	VkDescriptorSetLayout	vk_descriptorset_layout_sampler_;

	VkDescriptorSet         vk_descriptorset_uniform_;
	VkDescriptorSet			vk_descriptorset_sampler_;

	// pipeline
	VkPipelineLayout		vk_pipeline_layout_;

	VkPipeline				vk_pipeline_tex_;
	VkPipeline				vk_pipeline_flat_;
	VkPipeline				vk_pipeline_normal_mapping_;

	render_mode_t			render_mode_;

	void					SetRenderMode(render_mode_t mode);

	void					KeyF2Down() override;

	bool					LoadTexture(const char * filename, VkFormat format,
								VkImageUsageFlags image_usage, texture_s & tex);
	void					FreeTexture(texture_s& tex);

	bool					LoadTextures();
	void					FreeTextures();

	bool					CreaetSampler();
	void					DestroySampler();

	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	// vertex buffer
	bool					CreateVertexBuffer();
	void					DestroyVertexBuffer();

	// index buffer
	bool					CreateIndexBuffer();
	void					DestroyIndexBuffer();

	// descriptor set layout
	bool					CreateDescriptorSetLayout_Uniform();
	bool					CreateDescriptorSetLayout_Sampler();

	// descriptor set
	bool					AllocDescriptorSets();
	void					FreeDescriptorSets();

	// pipeline
	bool					CreatePipelineLayout();

	bool					CreatePipeline_Tex();
	bool					CreatePipeline_Flat();
	bool					CreatePipeline_NormalMapping();

	// build command buffer
	void					BuildCommandBuffer(VkPipeline pipeline);

	void					UpdateMVPUniformBuffer();
	void					SetupLightUniformBuffer();

	void					UpdateUniformBuffer(buffer_s& buffer, const std::byte* host_data, size_t host_data_size);

};

