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
	void					BuildCommandBuffers() override;
	void					Update() override;

private:

	enum class render_mode_t {
		RM_TEXTURE,
		RM_FLAT_LIGHT,
		RM_NORMAL_MAPPING
	};

	// sampler
	VkSampler				vk_sampler_;

	vk_image_s				texture_color_;
	vk_image_s				texture_normal_;

	vk_buffer_s				uniform_buffer_mat_;
	vk_buffer_s				uniform_buffer_point_light_;
	vk_buffer_s				vertex_buffer_;
	vk_buffer_s				index_buffer_;
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

	void					FuncKeyDown(uint32_t key) override;

	bool					LoadTextures();
	void					FreeTextures();

	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	// vertex buffer
	bool					CreateWall();
	void					DestroyWall();

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

};

