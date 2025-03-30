/******************************************************************************
 shadow map
 *****************************************************************************/

#pragma once

/*
================================================================================
ShadowMapDemo
================================================================================
*/
class ShadowMapDemo : public VkDemo {
public:

	ShadowMapDemo();
	~ShadowMapDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					Update();

protected:

	void					FuncKeyDown(uint32_t key) override;

private:

	static const uint32_t	FRAMEBUFFER_DIM = 1024;
	static const vertex_format_t	MODEL_EXPECT_VF = vertex_format_t::VF_POS_NORMAL_UV;

	static constexpr float	Z_NEAR = 2.0f;
	static constexpr float	Z_FAR = 32.0f;

	uint32_t				old_view_cx_;
	uint32_t				old_view_cy_;

	glm::vec3				light_dir_;

	VkModel					model_floor_;
	VkModel					model_object_;

	VkSampler				vk_sampler_depth_;
	VkFormat				vk_depth_format_;

	VkRenderPass			vk_render_pass_depth_;

	vk_image_s				vk_image_depth_;

	VkFramebuffer			vk_framebuffer_depth_;

	vk_buffer_s				vk_ubo_mvp_overlay_;
	vk_buffer_s				vk_ubo_mvp_light_;
	vk_buffer_s				vk_ubo_mvp_sep_;
	vk_buffer_s				vk_ubo_mvp_fixed_light_;
	vk_buffer_s				vk_ubo_mvp_sep_fixed_;
	vk_buffer_s				vk_ubo_viewer_;
	vk_buffer_s				vk_ubo_light_;

	// vbo
	vk_buffer_s				vk_vbo_overlay_;

	VkDescriptorSetLayout	vk_desc_set_layout_ubo4_tex_;
	VkDescriptorSetLayout	vk_desc_set_layout_ubo_tex_;

	VkDescriptorSet			vk_desc_set_overlay_;

	VkDescriptorSet			vk_desc_set_mvp_light_;	// for depth pass
	VkDescriptorSet			vk_desc_set_mvp_viewer_light_;

	VkDescriptorSet			vk_desc_set_mvp_fixed_light_;	// for depth pass
	VkDescriptorSet			vk_desc_set_mvp_viewer_light_fixed_;

	VkPipelineLayout		vk_pipeline_layout_;

	VkPipeline				vk_pipeline_overlay_;
	VkPipeline				vk_pipeline_depth_;
	VkPipeline				vk_pipeline_lighting_mat_;
	VkPipeline				vk_pipeline_lighting_tex_;

	bool					draw_overlay_;
	bool					draw_model_;
	
	bool					CreateUniformBuffers();

	bool					CreateSampler_Depth();

	bool					CreateRenderPass_Depth();

	bool					CreateDepthImage();

	bool					CreateDepthFramebuffer();

	bool					CreateDescriptorSetLayout_UBO4_Tex();
	bool					CreateDescriptorSetLayout_UBO_Tex();

	bool					AllocateDestriptorSets();
	void					FreeDescriptorSets();

	bool					CreateOverlayVBO();

	bool					CreatePipelineLayout();

	bool					CreatePipeline_Overlay();
	bool					CreatePipeline_Depth();
	bool					CreatePipeline_Lighting_Mat();
	bool					CreatePipeline_Lighting_Tex();

	void					SetupUBOs();
	void					UpdateBuffersForOverlay();
};
