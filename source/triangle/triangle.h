/******************************************************************************
 triangle
 *****************************************************************************/

#pragma once

/*
================================================================================
TriangleDemo
================================================================================
*/
class TriangleDemo : public VkDemo {
public:
	TriangleDemo();
	~TriangleDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					Update() override;

private:

	vk_buffer_s				uniform_buffer_mvp_;

	vk_buffer_s				vertex_buffer_;

	// descriptor
	VkDescriptorSetLayout   vk_descriptorset_layout_;
	VkDescriptorSet         vk_descriptorset_;

	// pipeline
	VkPipelineLayout		vk_pipeline_layout_;

	VkPipeline				vk_pipeline_;

	// uniform buffer
	bool					CreateUniformBuffer();
	void					DestroyUniformBuffer();

	// vertex buffer
	bool					CreateTriangle();
	void					DestroyTriangle();

	// descriptor set layout
	bool					CreateDescriptorSetLayout();

	// descriptor set
	bool					AllocDescriptorSets();
	void					FreeDescriptorSets();

	// pipeline
	bool					CreatePipelineLayout();

	bool					CreatePipeline();

	void					UpdateUniformBuffer();

};
