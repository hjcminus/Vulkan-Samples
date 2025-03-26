/******************************************************************************
 mesh shader
 *****************************************************************************/

#pragma once

/*
================================================================================
MeshShaderDemo
================================================================================
*/
class MeshShaderDemo : public VkDemo {
public:
	MeshShaderDemo();
	~MeshShaderDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					Update() override;

private:

	struct ubo_terrain_s {
		int32_t				vertex_count_per_edge_;
	};

	// VkDeviceCreateInfo::pNext chain
	VkPhysicalDeviceMeshShaderFeaturesEXT	vk_device_create_next_;

	// mesh shader draw call
	PFN_vkCmdDrawMeshTasksEXT	vkCmdDrawMeshTasksEXT;

	// descriptor
	VkDescriptorSetLayout   vk_descriptorset_layout_;
	VkDescriptorSet         vk_descriptorset_;

	VkDescriptorSetLayout   vk_descriptorset_layout2_;
	VkDescriptorSet         vk_descriptorset2_;

	// pipeline
	VkPipelineLayout        vk_pipeline_layout_;

	VkPipeline              vk_pipeline_wireframe_;
	VkPipeline				vk_pipeline_fill_;

	vk_buffer_s				uniform_buffer_mvp_;
	vk_buffer_s				uniform_buffer_terrain_;
	vk_buffer_s				shader_storage_buffer_heights_;
	vk_buffer_s				shader_storage_buffer_color_table_;

	bool					wireframe_mode_;
	uint32_t				random_seed_;
	static const terrain_size_t	TERRAIN_SIZE = terrain_size_t::TS_512;
	static const int		TERRAIN_MAX_Z = 64;
	uint32_t				vertex_count_per_edge_;

	void					AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) const override;
	void					AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) const override;

	void					FuncKeyDown(uint32_t key) override;

	// uniform buffer
	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	bool					UpdateTerrainUBO();

	// shader storage buffers
	bool					CreateShaderStorageBuffers();
	void					DestroyShaderStorageBuffers();

	// descriptor set layout
	bool					CreateDescriptorSetLayout();

	// descriptor set
	bool					AllocDemoDescriptorSet();
	void					FreeDemoDescriptorSet();

	// descriptor set layout2
	bool					CreateDescriptorSetLayout2();

	// descriptor set2
	bool					AllocDemoDescriptorSet2();
	void					FreeDemoDescriptorSet2();

	// pipeline layout
	bool					CreatePipelineLayout();

	// pipeline
	bool					CreatePipelines();

	void					UpdateTerrain();

	// build command buffer
	void					BuildCommandBuffer(VkPipeline pipeline);

	void					UpdateMVPUniformBuffer();
};
