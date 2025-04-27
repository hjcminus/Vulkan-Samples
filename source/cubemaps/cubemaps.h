/******************************************************************************
 cubemaps
 *****************************************************************************/

#pragma once

/*
================================================================================
CubeMapsDemo
================================================================================
*/
class CubeMapsDemo : public VkDemo {
public:
	CubeMapsDemo();
	~CubeMapsDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					WindowSizeChanged() override;
	void					Update() override;

private:

	// sampler
	VkSampler				vk_sampler_scene_;

	// texture
	vk_image_s				tex_cubemap_;

	// point light
	ubo_point_light_s		point_light_;

	// ubo
	vk_buffer_s				ubo_mvp_;
	vk_buffer_s				ubo_light_;
	vk_buffer_s				ubo_viewer_;

	// vbo
	vk_buffer_s				vbo_box_;

	VkModel					model_;

	// descriptor
	VkDescriptorSetLayout	vk_desc_set_layout_mvp_cubemap_;
	VkDescriptorSet			vk_desc_set_mvp_cubemap_;

	// pipeline
	VkPipelineLayout		vk_pipeline_layout_skybox_;
	VkPipelineLayout		vk_pipeline_layout_model_;

	VkPipeline				vk_pipeline_skybox_;
	VkPipeline				vk_pipeline_model_;

	bool					CreateBox();
	void					DestroyBox();

	bool					LoadModel();
	void					FreeModel();

	// uniform buffer
	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	bool					CreateDescSetLayouts();

	bool					AllocDescriptorSets();
	void					FreeDescriptorSets();

	bool					CreatePipelineLayouts();
	bool					CreatePipelines();

	void					SetupLight();
};
