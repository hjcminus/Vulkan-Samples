/******************************************************************************
 mesh shader
 *****************************************************************************/

#include "../common/inc.h"
#include "mesh_shader.h"

/*
================================================================================
MeshShaderDemo
================================================================================
*/
MeshShaderDemo::MeshShaderDemo():
	vkCmdDrawMeshTasksEXT(nullptr),
	vk_descriptorset_layout_(VK_NULL_HANDLE),
	vk_descriptorset_(VK_NULL_HANDLE),
	vk_descriptorset_layout2_(VK_NULL_HANDLE),
	vk_descriptorset2_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_wireframe_(VK_NULL_HANDLE),
	vk_pipeline_fill_(VK_NULL_HANDLE),
	wireframe_mode_(true),
	random_seed_(1)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Mesh shader (F2: toggle wireframe & fill mode)");
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

	memset(&uniform_buffer_mvp_, 0, sizeof(uniform_buffer_mvp_));
	memset(&uniform_buffer_terrain_, 0, sizeof(uniform_buffer_terrain_));
	memset(&shader_storage_buffer_heights_, 0, sizeof(shader_storage_buffer_heights_));
	memset(&shader_storage_buffer_color_table_, 0, sizeof(shader_storage_buffer_color_table_));

	vertex_count_per_edge_ = Terrain_GetVertexCountPerEdge(TERRAIN_SIZE);

	// init camera
	float ctr_xy = (vertex_count_per_edge_ - 1) * 0.5f;

	camera_.pos_ = glm::vec3(ctr_xy, ctr_xy, (vertex_count_per_edge_ - 1) * 0.5f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	CursorRotate(0.0f, -45.0f);
}

MeshShaderDemo::~MeshShaderDemo() {
	// do nothing
}

bool MeshShaderDemo::Init() {
	if (!VkDemo::Init("mesh_shader" /* shader files directory */,
		3, 2, 0, 2)) {
		return false;
	}

	// get mesh draw call
	vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(vk_device_, "vkCmdDrawMeshTasksEXT");
	if (!vkCmdDrawMeshTasksEXT) {
		printf("could not get function pointer of vkCmdDrawMeshTasksEXT\n");
		return false;
	}

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!UpdateTerrainUBO()) {
		return false;
	}

	if (!CreateShaderStorageBuffers()) {
		return false;
	}

	UpdateTerrain();

	if (!CreateDescriptorSetLayout()) {
		return false;
	}

	if (!AllocDemoDescriptorSet()) {
		return false;
	}

	if (!CreateDescriptorSetLayout2()) {
		return false;
	}

	if (!AllocDemoDescriptorSet2()) {
		return false;
	}

	if (!CreatePipelineLayout()) {
		return false;
	}

	if (!CreatePipelines()) {
		return false;
	}

	BuildCommandBuffer(vk_pipeline_wireframe_);

	enable_display_ = true;

	printf("F2: switch fill & line mode\n");
	printf("F3: update terrain\n");

	return true;
}

void MeshShaderDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_fill_);
	DestroyPipeline(vk_pipeline_wireframe_);
	DestroyPipelineLayout(vk_pipeline_layout_);
	FreeDemoDescriptorSet2();
	DestroyDescriptorSetLayout(vk_descriptorset_layout2_);
	FreeDemoDescriptorSet();
	DestroyDescriptorSetLayout(vk_descriptorset_layout_);
	DestroyShaderStorageBuffers();
	DestroyUniformBuffers();

	VkDemo::Shutdown();
}

void MeshShaderDemo::BuildCommandBuffers() {
	BuildCommandBuffer(wireframe_mode_ ? vk_pipeline_wireframe_ : vk_pipeline_fill_);
}

void MeshShaderDemo::Update() {
	UpdateMVPUniformBuffer();
}

void MeshShaderDemo::AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) const {
	// required by mesh shader
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
}

void MeshShaderDemo::AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) const {
	// required by mesh shader
	extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// required by VK_KHR_spirv_1_4
	extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

void MeshShaderDemo::FuncKeyDown(uint32_t key) {
	if (key == KEY_F2) {
		wireframe_mode_ = !wireframe_mode_;
		BuildCommandBuffer(wireframe_mode_ ? vk_pipeline_wireframe_ : vk_pipeline_fill_);
	}
	else if (key == KEY_F3) {
		UpdateTerrain();
	}
}

// uniform buffer
bool MeshShaderDemo::CreateUniformBuffers() {
	return CreateBuffer(uniform_buffer_mvp_, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ubo_mat_s))
		&&
		CreateBuffer(uniform_buffer_terrain_, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_terrain_s));
}

void MeshShaderDemo::DestroyUniformBuffers() {
	DestroyBuffer(uniform_buffer_terrain_);
	DestroyBuffer(uniform_buffer_mvp_);
}

bool MeshShaderDemo::UpdateTerrainUBO() {
	ubo_terrain_s ubo_terrain = {};
	ubo_terrain.vertex_count_per_edge_ = vertex_count_per_edge_;

	// update ubo_terrain
	return UpdateBuffer(uniform_buffer_terrain_, &ubo_terrain, sizeof(ubo_terrain));
}

// shader storage buffer
bool MeshShaderDemo::CreateShaderStorageBuffers() {
	// -- shader storage buffer of height values --

	uint32_t height_buffer_size = (uint32_t)(sizeof(float) * SQUARE(vertex_count_per_edge_));

	if (!CreateBuffer(shader_storage_buffer_heights_, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		height_buffer_size)) {
		return false;
	}

	// -- shader storage buffer of color table --
	
	glm::vec4 color_table[TERRAIN_MAX_Z + 1];

	rgb_s rgb_start = { 0.0, 0.0, 1.0 };	// blue
	rgb_s rgb_stop  = { 1.0, 0.0, 0.0 };	// red

	for (int i = 0; i <= TERRAIN_MAX_Z; ++i) {
		double t = i / (float)TERRAIN_MAX_Z;
		rgb_s rgb_int;
		RGBInterpolate(rgb_start, rgb_stop, t, rgb_int);
		color_table[i] = glm::vec4((float)rgb_int.r_, (float)rgb_int.g_, (float)rgb_int.b_, 1.0f /* opaque */);
	}

	if (!CreateBuffer(shader_storage_buffer_color_table_, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(color_table))) {
		return false;
	}

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, shader_storage_buffer_color_table_.memory_, 0,
		shader_storage_buffer_color_table_.memory_size_, 0 /*flags*/, &data)) {
		printf("vkMapMemory error\n");
		return false;
	}

	memcpy(data, color_table, sizeof(color_table));

	vkUnmapMemory(vk_device_, shader_storage_buffer_color_table_.memory_);

	return true;
}

void MeshShaderDemo::DestroyShaderStorageBuffers() {
	DestroyBuffer(shader_storage_buffer_color_table_);
	DestroyBuffer(shader_storage_buffer_heights_);
}

// descriptor set layout
bool MeshShaderDemo::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayoutBinding, 4> binding;

	int idx = 0;
	binding[idx].binding = 0;
	binding[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding[idx].descriptorCount = 1;
	binding[idx].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;    // descriptset can be used in which shaders
	binding[idx].pImmutableSamplers = nullptr;

	idx++;
	binding[idx].binding = 1;
	binding[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding[idx].descriptorCount = 1;
	binding[idx].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
	binding[idx].pImmutableSamplers = nullptr;

	idx++;
	binding[idx].binding = 2;
	binding[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding[idx].descriptorCount = 1;
	binding[idx].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
	binding[idx].pImmutableSamplers = nullptr;

	idx++;
	binding[idx].binding = 3;
	binding[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding[idx].descriptorCount = 1;
	binding[idx].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
	binding[idx].pImmutableSamplers = nullptr;

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)binding.size();
	create_info.pBindings = binding.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_);
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

	std::array<VkWriteDescriptorSet, 4> write_descriptor_set;

	int idx = 0;
	write_descriptor_set[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[idx].pNext = nullptr;
	write_descriptor_set[idx].dstSet = vk_descriptorset_;
	write_descriptor_set[idx].dstBinding = 0;    // layout (binding = 0) uniform BufferMat 
	write_descriptor_set[idx].dstArrayElement = 0;
	write_descriptor_set[idx].descriptorCount = 1;
	write_descriptor_set[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	write_descriptor_set[idx].pImageInfo = nullptr;
	write_descriptor_set[idx].pBufferInfo = &uniform_buffer_mvp_.desc_buffer_info_;
	write_descriptor_set[idx].pTexelBufferView = nullptr;

	idx++;
	write_descriptor_set[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[idx].pNext = nullptr;
	write_descriptor_set[idx].dstSet = vk_descriptorset_;
	write_descriptor_set[idx].dstBinding = 1;
	write_descriptor_set[idx].dstArrayElement = 0;
	write_descriptor_set[idx].descriptorCount = 1;
	write_descriptor_set[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	write_descriptor_set[idx].pImageInfo = nullptr;
	write_descriptor_set[idx].pBufferInfo = &uniform_buffer_terrain_.desc_buffer_info_;
	write_descriptor_set[idx].pTexelBufferView = nullptr;

	idx++;
	write_descriptor_set[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[idx].pNext = nullptr;
	write_descriptor_set[idx].dstSet = vk_descriptorset_;
	write_descriptor_set[idx].dstBinding = 2;		// layout(std430, binding = 1) buffer Vertices
	write_descriptor_set[idx].dstArrayElement = 0;
	write_descriptor_set[idx].descriptorCount = 1;
	write_descriptor_set[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_set[idx].pImageInfo = nullptr;
	write_descriptor_set[idx].pBufferInfo = &shader_storage_buffer_heights_.desc_buffer_info_;
	write_descriptor_set[idx].pTexelBufferView = nullptr;

	idx++;
	write_descriptor_set[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[idx].pNext = nullptr;
	write_descriptor_set[idx].dstSet = vk_descriptorset_;
	write_descriptor_set[idx].dstBinding = 3;
	write_descriptor_set[idx].dstArrayElement = 0;
	write_descriptor_set[idx].descriptorCount = 1;
	write_descriptor_set[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_set[idx].pImageInfo = nullptr;
	write_descriptor_set[idx].pBufferInfo = &shader_storage_buffer_color_table_.desc_buffer_info_;
	write_descriptor_set[idx].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(vk_device_, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

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

// descriptor set layout2
bool MeshShaderDemo::CreateDescriptorSetLayout2() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayoutBinding, 1> binding;

	int idx = 0;
	binding[idx].binding = 0;
	binding[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding[idx].descriptorCount = 1;
	binding[idx].stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT;    // descriptset can be used in which shaders
	binding[idx].pImmutableSamplers = nullptr;

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)binding.size();
	create_info.pBindings = binding.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout2_);
}

// descriptor set2
bool MeshShaderDemo::AllocDemoDescriptorSet2() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &vk_descriptorset_layout2_;

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, &vk_descriptorset2_)) {
		return false;
	}

	std::array<VkWriteDescriptorSet, 1> write_descriptor_set;

	int idx = 0;
	write_descriptor_set[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[idx].pNext = nullptr;
	write_descriptor_set[idx].dstSet = vk_descriptorset2_;
	write_descriptor_set[idx].dstBinding = 0;
	write_descriptor_set[idx].dstArrayElement = 0;
	write_descriptor_set[idx].descriptorCount = 1;
	write_descriptor_set[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	write_descriptor_set[idx].pImageInfo = nullptr;
	write_descriptor_set[idx].pBufferInfo = &uniform_buffer_terrain_.desc_buffer_info_;
	write_descriptor_set[idx].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(vk_device_, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

	return true;
}

void MeshShaderDemo::FreeDemoDescriptorSet2() {
	if (vk_descriptorset2_) {
		vkFreeDescriptorSets(vk_device_,
			vk_descriptor_pool_,
			1,
			&vk_descriptorset2_);
		vk_descriptorset2_ = VK_NULL_HANDLE;
	}
}

// pipeline layout
bool MeshShaderDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	VkDescriptorSetLayout descriptorset_layouts[2] = {
		vk_descriptorset_layout_, vk_descriptorset_layout2_
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 2; // use two descriptorset layout
	create_info.pSetLayouts = descriptorset_layouts;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info, nullptr, &vk_pipeline_layout_);
}

// pipeline
bool MeshShaderDemo::CreatePipelines() {
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
		.polygonMode = VK_POLYGON_MODE_LINE,
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

	VkResult rt1 = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_wireframe_);

	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	VkResult rt2 = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_fill_);

	// free shader modules
	vkDestroyShaderModule(vk_device_, fragment_shader, nullptr);
	vkDestroyShaderModule(vk_device_, mesh_shader, nullptr);
	vkDestroyShaderModule(vk_device_, task_shader, nullptr);

	return rt1 == VK_SUCCESS && rt2 == VK_SUCCESS;
}

void MeshShaderDemo::UpdateTerrain() {
	SRand(random_seed_++);

	// -- shader storage buffer of height values --

	terrain_gen_params_s terrain_gen_params = {
		.algo_ = terrain_gen_algorithm_t::MID_POINT,
		.sz_ = TERRAIN_SIZE,
		.min_z_ = 0.0f,
		.max_z_ = TERRAIN_MAX_Z,
		.iterations_ = 0,	// not used
		.filter_ = 0.0f,		// not used
		.roughness_ = 0.75f,	// 0.25 ~ 1.5
	};

	terrain_s test_terrain = {};
	// if sz value changed we need change camera position & value in task shader also

	if (!Terrain_Generate(terrain_gen_params, test_terrain)) {
		printf("Could not generate a test terrain\n");
		return;
	}

	uint32_t height_buffer_size = (uint32_t)(sizeof(float) * SQUARE(test_terrain.vertex_count_per_edge_));

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, shader_storage_buffer_heights_.memory_, 0,
		shader_storage_buffer_heights_.memory_size_, 0 /*flags*/, &data)) {

		Terrain_Free(test_terrain);

		printf("vkMapMemory error\n");
		return;
	}

	memcpy(data, test_terrain.heights_, height_buffer_size);

	// now we can free terrain memory
	Terrain_Free(test_terrain);

	vkUnmapMemory(vk_device_, shader_storage_buffer_heights_.memory_);
}

// build command buffer
void MeshShaderDemo::BuildCommandBuffer(VkPipeline pipeline) {
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
	// render_pass_begin_info.framebuffer
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = cfg_viewport_cx_;
	render_pass_begin_info.renderArea.extent.height = cfg_viewport_cy_;

	render_pass_begin_info.clearValueCount = 2; // color, depth-stencil
	render_pass_begin_info.pClearValues = clear_values;

	uint32_t sz_draw_cmd_buffer = (uint32_t)vk_draw_cmd_buffer_count_;
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
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

		VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = cfg_viewport_cx_;
		scissor.extent.height = cfg_viewport_cy_;

		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

		VkDescriptorSet descriptor_sets[2] = { vk_descriptorset_, vk_descriptorset2_ };

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, 2, descriptor_sets, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// draw
		vkCmdDrawMeshTasksEXT(cmd_buf, 1, 1, 1);

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void MeshShaderDemo::UpdateMVPUniformBuffer() {
	ubo_mat_s ubo_mvp;

	glm::mat4 projection_matrix = glm::perspective(glm::radians(70.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 4.0f, 4096.0f);

	glm::mat4 view_matrix;
	GetViewMatrix(view_matrix);

	glm::mat4 model_matrix;
	GetModelMatrix(model_matrix);

	ubo_mvp.matrix_ = projection_matrix * view_matrix * model_matrix;

	UpdateBuffer(uniform_buffer_mvp_, &ubo_mvp, sizeof(ubo_mvp));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(MeshShaderDemo)
