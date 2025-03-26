/******************************************************************************
 instancing
 *****************************************************************************/

#pragma once

/*
================================================================================
InstancingDemo
================================================================================
*/
class InstancingDemo : public VkDemo {
public:
	InstancingDemo();
	~InstancingDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					Update() override;

private:

	const float				VIEW_DISTANCE = 256.0f;
	static const uint32_t	INSTANCE_COUNT = 512;

	vk_buffer_s				ubo_mvp_;
	vk_buffer_s				ubo_viewer_;
	vk_buffer_s				ubo_light_;

	vk_buffer_s				buf_instance_;

	VkModel					model_;

	// descriptor
	VkDescriptorSetLayout   vk_desc_set_layout_;

	VkDescriptorSet			vk_desc_set_;

	// pipeline
	VkPipelineLayout		vk_pipeline_layout_;
	VkPipeline				vk_pipeline_;

	// uniform buffer
	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	bool					LoadModel();
	void					FreeModel();

	bool					CreateInstanceBuffer();
	void					DestroyInstanceBuffer();

	// descriptor set layout
	bool					CreateDescSetLayout();

	// descriptor set
	bool					AllocDescriptorSets();
	void					FreeDescriptorSets();

	// pipeline
	bool					CreatePipelineLayout();
	bool					CreatePipeline();

	void					UpdateMVPUniformBuffer();
	void					SetupViewerUniformBuffer();
	void					SetupLightUniformBuffer();
};
