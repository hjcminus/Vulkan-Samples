/******************************************************************************
 mesh shader
 *****************************************************************************/

#include "vk_demo.h"

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
	void					Display() override;

private:

	// uniform buffer & shader storage buffer
	struct buffer_s {
		VkDeviceMemory		memory_;
		VkBuffer			buffer_;
		VkDescriptorBufferInfo  buffer_info_;
	};

	struct ubo_s {
		glm::mat4			model_view_proj_mat_;
	};

	// VkDeviceCreateInfo::pNext chain
	VkPhysicalDeviceMeshShaderFeaturesEXT	vk_device_create_next_;

	// mesh shader draw call
	PFN_vkCmdDrawMeshTasksEXT	vkCmdDrawMeshTasksEXT;
	
	// descriptor
	VkDescriptorPool        vk_descriptor_pool_;
	VkDescriptorSetLayout   vk_descriptorset_layout_;
	VkDescriptorSet         vk_descriptorset_;

	// pipeline
	VkPipelineCache         vk_pipeline_cache_;
	VkPipelineLayout        vk_pipeline_layout_;
	VkPipeline              vk_pipeline_;

	buffer_s				uniform_buffer_;
	buffer_s				shader_storage_buffer_;

	void					AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) override;
	void					AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) override;

	// uniform buffer
	bool					CreateUniformBuffer();
	void					DestroyUniformBuffer();

	// shader storage buffer
	bool					CreateShaderStorageBuffer();
	void					DestroyShaderStorageBuffer();

	// descriptor set pool
	bool					CreateDescriptorPool();
	void					DestroyDescriptorPool();

	// descriptor set layout
	bool					CreateDescriptorSetLayout();
	void					DestroyDescriptorSetLayout();

	// descriptor set
	bool					AllocDemoDescriptorSet();
	void					FreeDemoDescriptorSet();

	// pipeline cache
	bool					CreatePipelineCache();
	void					DestroyPipelineCache();

	// pipeline layout
	bool					CreatePipelineLayout();
	void					DestroyPipelineLayout();

	// pipeline
	bool					CreatePipeline();
	void					DestroyPipeline();

	// build command buffer
	void					BuildCommandBuffer();

	void					UpdateUniformBuffers();

};

MeshShaderDemo::MeshShaderDemo():
	vkCmdDrawMeshTasksEXT(nullptr),
	vk_descriptor_pool_(VK_NULL_HANDLE),
	vk_descriptorset_layout_(VK_NULL_HANDLE),
	vk_descriptorset_(VK_NULL_HANDLE),
	vk_pipeline_cache_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_(VK_NULL_HANDLE)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Mesh shader");
#endif

	vk_device_create_next_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	vk_device_create_next_.pNext = nullptr;
	vk_device_create_next_.taskShader = VK_TRUE;	// enable task shader
	vk_device_create_next_.meshShader = VK_TRUE;	// enable mesh shader
	vk_device_create_next_.multiviewMeshShader = VK_FALSE;
	vk_device_create_next_.primitiveFragmentShadingRateMeshShader = VK_FALSE;
	vk_device_create_next_.meshShaderQueries = VK_FALSE;

	// will assign to VkDeviceCreateInfo::pNext field to enable task & mesh shaders
	vk_device_create_next_chain_ = &vk_device_create_next_;

	memset(&uniform_buffer_, 0, sizeof(uniform_buffer_));
	memset(&shader_storage_buffer_, 0, sizeof(shader_storage_buffer_));
}

MeshShaderDemo::~MeshShaderDemo() {
	// do nothing
}

bool MeshShaderDemo::Init() {
	if (!VkDemo::Init("mesh_shader" /* shader files directory */)) {
		return false;
	}

	// get mesh draw call
	vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(vk_device_, "vkCmdDrawMeshTasksEXT");
	if (!vkCmdDrawMeshTasksEXT) {
		printf("could not get function pointer of vkCmdDrawMeshTasksEXT\n");
		return false;
	}

	if (!CreateUniformBuffer()) {
		return false;
	}

	if (!CreateShaderStorageBuffer()) {
		return false;
	}

	if (!CreateDescriptorPool()) {
		return false;
	}

	if (!CreateDescriptorSetLayout()) {
		return false;
	}

	if (!AllocDemoDescriptorSet()) {
		return false;
	}

	if (!CreatePipelineCache()) {
		return false;
	}

	if (!CreatePipelineLayout()) {
		return false;
	}

	if (!CreatePipeline()) {
		return false;
	}

	BuildCommandBuffer();

	enable_display_ = true;

	return true;
}

void MeshShaderDemo::Shutdown() {
	DestroyPipeline();
	DestroyPipelineLayout();
	DestroyPipelineCache();
	FreeDemoDescriptorSet();
	DestroyDescriptorSetLayout();
	DestroyDescriptorPool();
	DestroyShaderStorageBuffer();
	DestroyUniformBuffer();

	VkDemo::Shutdown();
}

void MeshShaderDemo::Display() {
	static int g_display_count = 0;
	g_display_count++;

	uint32_t current_image_idx = 0;
	VkResult rt;

	UpdateUniformBuffers();

	// If semaphore is not VK_NULL_HANDLE, it must not have any uncompleted signal or wait
	//   operations pending
	// If fence is not VK_NULL_HANDLE, fence must be unsignaled
	// semaphore and fence must not both be equal to VK_NULL_HANDLE
	rt = vkAcquireNextImageKHR(vk_device_, vk_swapchain_, UINT64_MAX /* never timeout */,
		vk_semaphore_present_complete_, VK_NULL_HANDLE, &current_image_idx);

	// test_wait_semaphore(demo); // crash

	printf("Display frame = %d, current_image_idx = %u\n",
		g_display_count, current_image_idx);

	if (rt != VK_SUCCESS) {
		printf("vkAcquireNextImageKHR error\n");
		return;
	}

	VkFence fence = vk_wait_fences_[current_image_idx];

	vkWaitForFences(vk_device_, 1, &fence, VK_TRUE /* waitAll */, UINT64_MAX /* never timeout */);
	vkResetFences(vk_device_, 1, &fence);

	VkSubmitInfo submit_info = {};

	VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &vk_semaphore_present_complete_;
	submit_info.pWaitDstStageMask = &pipeline_stage_flags;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk_draw_cmd_buffers_[current_image_idx];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &vk_semaphore_render_complete_;

	// change command buffer state from executable to pending
	rt = vkQueueSubmit(vk_graphics_queue_, 1, &submit_info, fence);
	if (rt != VK_SUCCESS) {
		printf("vkQueueSubmit error\n");
		return;
	}

	VkPresentInfoKHR present_info = {};

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk_semaphore_render_complete_;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk_swapchain_;
	present_info.pImageIndices = &current_image_idx;
	present_info.pResults = nullptr;

	rt = vkQueuePresentKHR(vk_graphics_queue_, &present_info);
	if (rt != VK_SUCCESS) {
		printf("vkQueuePresentKHR error\n");
		return;
	}

	rt = vkQueueWaitIdle(vk_graphics_queue_);
	if (rt != VK_SUCCESS) {
		printf("vkQueueWaitIdle error\n");
		return;
	}
}

void MeshShaderDemo::AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) {
	// required by mesh shader
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
}

void MeshShaderDemo::AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) {
	// required by mesh shader
	extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// required by VK_KHR_spirv_1_4
	extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

// uniform buffer
bool MeshShaderDemo::CreateUniformBuffer() {
	VkBufferCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.size = sizeof(ubo_s);
	create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;  // is a pointer to an array of queue families that will access this buffer. It is
	// ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.

	if (VK_SUCCESS != vkCreateBuffer(vk_device_, &create_info, nullptr, &uniform_buffer_.buffer_)) {
		return false;
	}

	VkMemoryRequirements mem_req = {};
	vkGetBufferMemoryRequirements(vk_device_, uniform_buffer_.buffer_, &mem_req);

	VkMemoryAllocateInfo mem_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = mem_req.size,
		.memoryTypeIndex = GetMemoryTypeIndex(mem_req.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	};

	if (VK_SUCCESS != vkAllocateMemory(vk_device_, &mem_alloc_info, nullptr, &uniform_buffer_.memory_)) {
		return false;
	}

	if (VK_SUCCESS != vkBindBufferMemory(vk_device_, uniform_buffer_.buffer_, uniform_buffer_.memory_, 0)) {
		return false;
	}

	uniform_buffer_.buffer_info_.buffer = uniform_buffer_.buffer_;
	uniform_buffer_.buffer_info_.offset = 0;
	uniform_buffer_.buffer_info_.range = (VkDeviceSize)sizeof(ubo_s);

	return true;
}

void MeshShaderDemo::DestroyUniformBuffer() {
	if (uniform_buffer_.memory_) {
		vkFreeMemory(vk_device_, uniform_buffer_.memory_, nullptr);
		uniform_buffer_.memory_ = VK_NULL_HANDLE;
	}

	if (uniform_buffer_.buffer_) {
		vkDestroyBuffer(vk_device_, uniform_buffer_.buffer_, nullptr);
		uniform_buffer_.buffer_ = VK_NULL_HANDLE;
	}

	memset(&uniform_buffer_, 0, sizeof(uniform_buffer_));
}

// shader storage buffer
bool MeshShaderDemo::CreateShaderStorageBuffer() {
	std::vector<glm::vec4> vertex_buffer = {
		{  1.0f,  1.0f, 0.0f, 1.0f },
		{ -1.0f,  1.0f, 0.0f, 1.0f },
		{  0.0f, -1.0f, 0.0f, 1.0f },
	};
	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(glm::vec4);

	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = vertex_buffer_size,	// tune this
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	if (VK_SUCCESS != vkCreateBuffer(vk_device_, &create_info, nullptr, &shader_storage_buffer_.buffer_)) {
		return false;
	}

	VkMemoryRequirements memory_requierments;
	vkGetBufferMemoryRequirements(vk_device_, shader_storage_buffer_.buffer_, &memory_requierments);

	VkMemoryAllocateInfo memory_allocate_info = {};

	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = nullptr;
	memory_allocate_info.allocationSize = memory_requierments.size;
	memory_allocate_info.memoryTypeIndex = GetMemoryTypeIndex(memory_requierments.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (VK_SUCCESS != vkAllocateMemory(vk_device_, &memory_allocate_info, nullptr, &shader_storage_buffer_.memory_)) {
		printf("vkAllocateMemory error\n");
		return false;
	}

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, shader_storage_buffer_.memory_, 0, 
		memory_allocate_info.allocationSize, 0 /*flags*/, &data)) {
		printf("vkMapMemory error\n");
		return false;
	}

	memcpy(data, vertex_buffer.data(), vertex_buffer_size);

	vkUnmapMemory(vk_device_, shader_storage_buffer_.memory_);

	if (VK_SUCCESS != vkBindBufferMemory(vk_device_, 
		shader_storage_buffer_.buffer_, shader_storage_buffer_.memory_, 0 /* offset */)) {
		printf("vkBindBufferMemory error\n");
		return false;
	}

	shader_storage_buffer_.buffer_info_.buffer = shader_storage_buffer_.buffer_;
	shader_storage_buffer_.buffer_info_.offset = 0;
	shader_storage_buffer_.buffer_info_.range = vertex_buffer_size;

	return true;
}

void MeshShaderDemo::DestroyShaderStorageBuffer() {
	if (shader_storage_buffer_.memory_) {
		vkFreeMemory(vk_device_, shader_storage_buffer_.memory_, nullptr);
		shader_storage_buffer_.memory_ = VK_NULL_HANDLE;
	}

	if (shader_storage_buffer_.buffer_) {
		vkDestroyBuffer(vk_device_, shader_storage_buffer_.buffer_, nullptr);
		shader_storage_buffer_.buffer_ = VK_NULL_HANDLE;
	}

	memset(&shader_storage_buffer_, 0, sizeof(shader_storage_buffer_));
}

// descriptor set pool
bool MeshShaderDemo::CreateDescriptorPool() {
	VkDescriptorPoolCreateInfo create_info = {};

	VkDescriptorPoolSize pool_size_array[2];

	pool_size_array[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size_array[0].descriptorCount = 1;

	pool_size_array[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size_array[1].descriptorCount = 1;

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	create_info.maxSets = 1;    // is the maximum number of descriptor sets that can be allocated from the pool.
	create_info.poolSizeCount = 2;
	create_info.pPoolSizes = pool_size_array;

	return VK_SUCCESS == vkCreateDescriptorPool(vk_device_, &create_info, nullptr, &vk_descriptor_pool_);
}

void MeshShaderDemo::DestroyDescriptorPool() {
	if (vk_descriptor_pool_) {
		vkDestroyDescriptorPool(vk_device_, vk_descriptor_pool_, nullptr);
		vk_descriptor_pool_ = VK_NULL_HANDLE;
	}
}

// descriptor set layout
bool MeshShaderDemo::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	VkDescriptorSetLayoutBinding binding[2];

	binding[0].binding = 0;
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding[0].descriptorCount = 1;
	binding[0].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;    // descriptset can be used in which shaders
	binding[0].pImmutableSamplers = nullptr;

	binding[1].binding = 1;
	binding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding[1].descriptorCount = 1;
	binding[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
	binding[1].pImmutableSamplers = nullptr;

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = 2;
	create_info.pBindings = binding;

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_);
}

void MeshShaderDemo::DestroyDescriptorSetLayout() {
	if (vk_descriptorset_layout_) {
		vkDestroyDescriptorSetLayout(vk_device_, vk_descriptorset_layout_, nullptr);
		vk_descriptorset_layout_ = VK_NULL_HANDLE;
	}
}

// descriptor set
bool MeshShaderDemo::AllocDemoDescriptorSet() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &vk_descriptorset_layout_;

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, &vk_descriptorset_)) {
		return false;
	}

	VkWriteDescriptorSet write_descriptor_set[2];

	write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[0].pNext = nullptr;
	write_descriptor_set[0].dstSet = vk_descriptorset_;
	write_descriptor_set[0].dstBinding = 0;    // layout (binding = 0) uniform BufferMat 
	write_descriptor_set[0].dstArrayElement = 0;
	write_descriptor_set[0].descriptorCount = 1;
	write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	write_descriptor_set[0].pImageInfo = nullptr;
	write_descriptor_set[0].pBufferInfo = &uniform_buffer_.buffer_info_;
	write_descriptor_set[0].pTexelBufferView = nullptr;

	write_descriptor_set[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[1].pNext = nullptr;
	write_descriptor_set[1].dstSet = vk_descriptorset_;
	write_descriptor_set[1].dstBinding = 1;		// layout(std430, binding = 1) buffer Vertices
	write_descriptor_set[1].dstArrayElement = 0;
	write_descriptor_set[1].descriptorCount = 1;
	write_descriptor_set[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_set[1].pImageInfo = nullptr;
	write_descriptor_set[1].pBufferInfo = &shader_storage_buffer_.buffer_info_;
	write_descriptor_set[1].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(vk_device_, 2, write_descriptor_set, 0, nullptr);

	return true;
}

void MeshShaderDemo::FreeDemoDescriptorSet() {
	if (vk_descriptorset_) {
		vkFreeDescriptorSets(vk_device_,
			vk_descriptor_pool_,
			1,
			&vk_descriptorset_);
		vk_descriptorset_ = VK_NULL_HANDLE;
	}
}

bool MeshShaderDemo::CreatePipelineCache() {
	VkPipelineCacheCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.initialDataSize = 0;
	create_info.pInitialData = nullptr;

	return VK_SUCCESS == vkCreatePipelineCache(vk_device_, &create_info, nullptr, &vk_pipeline_cache_);
}

void MeshShaderDemo::DestroyPipelineCache() {
	if (vk_pipeline_cache_) {
		vkDestroyPipelineCache(vk_device_, vk_pipeline_cache_, nullptr);
		vk_pipeline_cache_ = VK_NULL_HANDLE;
	}
}

// pipeline layout
bool MeshShaderDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1; // use one descriptorset layout
	create_info.pSetLayouts = &vk_descriptorset_layout_;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info, nullptr, &vk_pipeline_layout_);
}

void MeshShaderDemo::DestroyPipelineLayout() {
	if (vk_pipeline_layout_) {
		vkDestroyPipelineLayout(vk_device_, vk_pipeline_layout_, nullptr);
		vk_pipeline_layout_ = VK_NULL_HANDLE;
	}
}

// pipeline
bool MeshShaderDemo::CreatePipeline() {
	VkShaderModule task_shader = VK_NULL_HANDLE, mesh_shader = VK_NULL_HANDLE, fragment_shader = VK_NULL_HANDLE;

	LoadShader("SPIR-V/base.task.spv", task_shader);
	LoadShader("SPIR-V/base.mesh.spv", mesh_shader);
	LoadShader("SPIR-V/base.frag.spv", fragment_shader);

	if (!task_shader || !mesh_shader || !fragment_shader) {
		if (task_shader) {
			vkDestroyShaderModule(vk_device_, task_shader, nullptr);
		}

		if (mesh_shader) {
			vkDestroyShaderModule(vk_device_, mesh_shader, nullptr);
		}

		if (fragment_shader) {
			vkDestroyShaderModule(vk_device_, fragment_shader, nullptr);
		}

		return false;
	}

	VkGraphicsPipelineCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 3> stages;

	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].pNext = nullptr;
	stages[0].flags = 0;
	stages[0].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
	stages[0].module = task_shader;
	stages[0].pName = "main";   // program entrance
	stages[0].pSpecializationInfo = nullptr;

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].pNext = nullptr;
	stages[1].flags = 0;
	stages[1].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
	stages[1].module = mesh_shader;
	stages[1].pName = "main";   // program entrance
	stages[1].pSpecializationInfo = nullptr;

	stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[2].pNext = nullptr;
	stages[2].flags = 0;
	stages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[2].module = fragment_shader;
	stages[2].pName = "main";   // program entrance
	stages[2].pSpecializationInfo = nullptr;

	create_info.stageCount = (uint32_t)stages.size();
	create_info.pStages = stages.data();

	// -- pVertexInputState --
	create_info.pVertexInputState = nullptr;	// no vertex input

	// -- pInputAssemblyState --
	create_info.pInputAssemblyState = nullptr;	// no input assemble

	// -- pTessellationState --
	create_info.pTessellationState = nullptr;   // no tessellation

	// -- pViewportState --
	VkPipelineViewportStateCreateInfo viewport_state = {};

	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = nullptr;    // overriden by dynamic state
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = nullptr;     // overriden by dynamic state

	create_info.pViewportState = &viewport_state;

	// -- pRasterizationState --
	VkPipelineRasterizationStateCreateInfo rasterization_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,    // fill mode
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,   // VK_FRONT_FACE_CLOCKWISE  VK_FRONT_FACE_COUNTER_CLOCKWISE
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

	create_info.pRasterizationState = &rasterization_state;

	// -- pMultisampleState --
	VkPipelineMultisampleStateCreateInfo multisample_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	create_info.pMultisampleState = &multisample_state;

	// -- pDepthStencilState --

	VkStencilOpState stencil_op_state = {
		.failOp = VK_STENCIL_OP_KEEP,
		.passOp = VK_STENCIL_OP_KEEP,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.compareMask = 0,
		.writeMask = 0,
		.reference = 0
	};

	VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = stencil_op_state,
		.back = stencil_op_state,
		.minDepthBounds = 0.0,
		.maxDepthBounds = 1.0
	};

	create_info.pDepthStencilState = &depthstencil_state;

	// -- pColorBlendState --
	VkPipelineColorBlendStateCreateInfo color_blend_state = {};

	VkPipelineColorBlendAttachmentState blend_attr_attachment = {};
	blend_attr_attachment.colorWriteMask = 0xf;
	blend_attr_attachment.blendEnable = VK_FALSE;

	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.pNext = nullptr;
	color_blend_state.flags = 0;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.logicOp = VK_LOGIC_OP_COPY;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &blend_attr_attachment;

	create_info.pColorBlendState = &color_blend_state;

	// -- pDynamicState --

	VkPipelineDynamicStateCreateInfo dynamic_state = {};

	std::vector<VkDynamicState> dynamic_state_enables;
	dynamic_state_enables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamic_state_enables.push_back(VK_DYNAMIC_STATE_SCISSOR);

	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = nullptr;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());
	dynamic_state.pDynamicStates = dynamic_state_enables.data();

	create_info.pDynamicState = &dynamic_state;

	create_info.layout = vk_pipeline_layout_;
	create_info.renderPass = vk_render_pass_;

	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = 0;

	VkResult rt = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_);

	// free shader modules
	vkDestroyShaderModule(vk_device_, fragment_shader, nullptr);
	vkDestroyShaderModule(vk_device_, mesh_shader, nullptr);
	vkDestroyShaderModule(vk_device_, task_shader, nullptr);

	return rt == VK_SUCCESS;
}

void MeshShaderDemo::DestroyPipeline() {
	if (vk_pipeline_) {
		vkDestroyPipeline(vk_device_, vk_pipeline_, nullptr);
		vk_pipeline_ = VK_NULL_HANDLE;
	}
}

// build command buffer
void MeshShaderDemo::BuildCommandBuffer() {
	VkCommandBufferBeginInfo cmd_buf_begin_info = {};

	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = 0;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo render_pass_begin_info = {};

	VkClearValue clear_values[2];

	clear_values[0].color.float32[0] = 0.0f;
	clear_values[0].color.float32[1] = 0.0f;
	clear_values[0].color.float32[2] = 0.0f;
	clear_values[0].color.float32[3] = 1.0f;
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;

	render_pass_begin_info.renderPass = vk_render_pass_;
	//render_pass_begin_info.framebuffer
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = cfg_viewport_cx_;
	render_pass_begin_info.renderArea.extent.height = cfg_viewport_cy_;

	render_pass_begin_info.clearValueCount = 2; // color, depth-stencil
	render_pass_begin_info.pClearValues = clear_values;

	uint32_t sz_draw_cmd_buffer = (uint32_t)vk_draw_cmd_buffers_.size();
	for (uint32_t i = 0; i < sz_draw_cmd_buffer; ++i) {
		VkCommandBuffer cmd_buf = vk_draw_cmd_buffers_[i];

		vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);

		render_pass_begin_info.framebuffer = vk_framebuffers_[i];

		vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {
			.x = 0.0f,
			.y = (float)cfg_viewport_cy_,
			.width = (float)cfg_viewport_cx_,
			.height = -(float)cfg_viewport_cy_,
			.minDepth = 0.0f,
			.maxDepth = 0.0f
		};
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

		VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = cfg_viewport_cx_;
		scissor.extent.height = cfg_viewport_cy_;

		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, 1, &vk_descriptorset_, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_);

		// draw
		vkCmdDrawMeshTasksEXT(cmd_buf, 1, 1, 1);

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void MeshShaderDemo::UpdateUniformBuffers() {
	ubo_s ubo;

	glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 0.1f, 256.0f);

	/*
		   y
		   |
		   |
		   |______x
		  /
		 /
		z

	*/

	glm::mat4 view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f),
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 model_matrix = glm::mat4(1.0f);

	ubo.model_view_proj_mat_ = projection_matrix * view_matrix * model_matrix;


	void* data = nullptr;
	VkResult rt = vkMapMemory(vk_device_, uniform_buffer_.memory_, 0, sizeof(ubo), 0, &data);
	if (rt != VK_SUCCESS) {
		printf("[UpdateUniformBuffers] vkMapMemory error\n");
		return;
	}

	memcpy(data, &ubo, sizeof(ubo));

	vkUnmapMemory(vk_device_, uniform_buffer_.memory_);
}

/*
================================================================================
entrance
================================================================================
*/
#include "../common/funcs.h"

int main(int argc, char ** argv) {
	Common_Init();

	MeshShaderDemo	demo;

	if (!demo.Init()) {
		demo.Shutdown();
		return 1;
	}

	demo.MainLoop();
	demo.Shutdown();

	return 0;
}
