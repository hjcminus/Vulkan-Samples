/******************************************************************************
 triangle
 *****************************************************************************/

#include "../common/inc.h"
#include "triangle.h"

/*
================================================================================
TriangleDemo
================================================================================
*/

TriangleDemo::TriangleDemo():
	vk_descriptorset_layout_(VK_NULL_HANDLE),
	vk_descriptorset_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_(VK_NULL_HANDLE)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Triangle");
#endif

	memset(&uniform_buffer_mvp_, 0, sizeof(uniform_buffer_mvp_));
}

TriangleDemo::~TriangleDemo() {
	// do nothing
}

bool TriangleDemo::Init() {
	if (!VkDemo::Init("triangle" /* shader files directory */)) {
		return false;
	}

	if (!CreateDescriptorPools(1, 0, 0, 1)) {
		return false;
	}

	if (!CreateUniformBuffer()) {
		return false;
	}

	if (!CreateVertexBuffer()) {
		return false;
	}

	if (!CreateDescriptorSetLayout()) {
		return false;
	}

	if (!AllocDescriptorSets()) {
		return false;
	}

	if (!CreatePipelineLayout()) {
		return false;
	}

	if (!CreatePipeline()) {
		return false;
	}

	BuildCommandBuffers();

	// init camera
	camera_mode_ = camera_mode_t::CM_ROTATE_OBJECT;

	/*
	      y
	 z   /
	 |  /
	 | /
	 |/______x

	 */

	// z axis is up
	camera_.pos_ = glm::vec3(0.0f, -2.0f, 0.0f);
	camera_.target_ = glm::vec3(0.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	move_speed_ = 0.1f;

	enable_display_ = true;

	return true;
}

void TriangleDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_);
	DestroyPipelineLayout(vk_pipeline_layout_);
	FreeDescriptorSets();
	DestroyDescriptorSetLayout(vk_descriptorset_layout_);
	DestroyVertexBuffer();
	DestroyUniformBuffer();
	DestroyDescriptorPools();

	VkDemo::Shutdown();
}

void TriangleDemo::Display() {
	uint32_t current_image_idx = 0;
	VkResult rt;

	UpdateUniformBuffer();

	// If semaphore is not VK_NULL_HANDLE, it must not have any uncompleted signal or wait
	//   operations pending
	// If fence is not VK_NULL_HANDLE, fence must be unsignaled
	// semaphore and fence must not both be equal to VK_NULL_HANDLE
	rt = vkAcquireNextImageKHR(vk_device_, vk_swapchain_, UINT64_MAX /* never timeout */,
		vk_semaphore_present_complete_, VK_NULL_HANDLE, &current_image_idx);

	if (rt != VK_SUCCESS) {
		printf("vkAcquireNextImageKHR error\n");
		return;
	}

	VkFence fence = vk_wait_fences_[current_image_idx];

	vkWaitForFences(vk_device_, 1, &fence, VK_TRUE /* waitAll */, UINT64_MAX /* never timeout */);
	vkResetFences(vk_device_, 1, &fence);	// set the state of current fence to unsignal.

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

void TriangleDemo::BuildCommandBuffers() {
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

		VkDescriptorSet descriptor_sets[1] = { vk_descriptorset_ };

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, 1, descriptor_sets, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_);

		VkDeviceSize offset[1] = { 0 };
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer_.buffer_, offset);


		/* When the command is executed, primitives are assembled using the current
		   primitive topology and vertexCount consecutive vertex indices with the
		   first vertexIndex value equal to firstVertex. The primitives are drawn
		   instanceCount times with instanceIndex starting with firstInstance and
		   increasing sequentially for each instance. The assembled primitives execute
		   the bound graphics pipeline.
		 */
		vkCmdDraw(cmd_buf,
			3,	//  vertexCount
			1,	//	instanceCount: is the number of instances to draw
			0,	//	firstVertex
			0);	//	firstInstance

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

// uniform buffer
bool TriangleDemo::CreateUniformBuffer() {
	return CreateBuffer(uniform_buffer_mvp_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ubo_mvp_s));
}

void TriangleDemo::DestroyUniformBuffer() {
	DestroyBuffer(uniform_buffer_mvp_);
}

// vertex buffer
bool TriangleDemo::CreateVertexBuffer() {
	std::vector<vertex_pos_color_s> vertex_buffer = {
		{ { -1.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  0.0f, 0.0f,  1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};
	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(vertex_pos_color_s);

	if (!CreateBuffer(vertex_buffer_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_buffer_size)) {
		return false;
	}

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, vertex_buffer_.memory_, 0, vertex_buffer_.memory_size_, 0, &data)) {
		return false;
	}

	memcpy(data, vertex_buffer.data(), vertex_buffer_size);

	vkUnmapMemory(vk_device_, vertex_buffer_.memory_);

	return true;
}

void TriangleDemo::DestroyVertexBuffer() {
	DestroyBuffer(vertex_buffer_);
}

// descriptor set layout
bool TriangleDemo::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	VkDescriptorSetLayoutBinding binding[1] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = 1;
	create_info.pBindings = binding;

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_);

}

// descriptor set
bool TriangleDemo::AllocDescriptorSets() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	VkDescriptorSetLayout set_layouts[1] = {
		vk_descriptorset_layout_
	};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = set_layouts;

	VkDescriptorSet sets[1] = {};

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, sets)) {
		return false;
	}

	vk_descriptorset_ = sets[0];

	std::array<VkWriteDescriptorSet, 1> write_descriptor_sets;

	int idx = 0;
	write_descriptor_sets[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[idx].pNext = nullptr;
	write_descriptor_sets[idx].dstSet = vk_descriptorset_;
	write_descriptor_sets[idx].dstBinding = 0;
	write_descriptor_sets[idx].dstArrayElement = 0;
	write_descriptor_sets[idx].descriptorCount = 1;
	write_descriptor_sets[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[idx].pImageInfo = nullptr;
	write_descriptor_sets[idx].pBufferInfo = &uniform_buffer_mvp_.buffer_info_;
	write_descriptor_sets[idx].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(vk_device_, (uint32_t)write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);

	return true;
}

void TriangleDemo::FreeDescriptorSets() {
	if (vk_descriptorset_) {
		VkDescriptorSet sets[1] = {
			vk_descriptorset_
		};
		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, 1, sets);

		vk_descriptorset_ = VK_NULL_HANDLE;
	}
}

// pipeline
bool TriangleDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	VkDescriptorSetLayout descriptorset_layouts[1] = {
		vk_descriptorset_layout_,	// order0: set = 0
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = descriptorset_layouts;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info, nullptr, &vk_pipeline_layout_);

}

bool TriangleDemo::CreatePipeline() {
	VkShaderModule vert_shader = VK_NULL_HANDLE, frag_shader = VK_NULL_HANDLE;

	LoadShader("SPIR-V/color.vert.spv", vert_shader);
	LoadShader("SPIR-V/color.frag.spv", frag_shader);

	if (!vert_shader || !frag_shader) {
		if (vert_shader) {
			vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
		}

		if (frag_shader) {
			vkDestroyShaderModule(vk_device_, frag_shader, nullptr);
		}
		return false;
	}

	VkGraphicsPipelineCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 2> stages;

	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].pNext = nullptr;
	stages[0].flags = 0;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vert_shader;
	stages[0].pName = "main";
	stages[0].pSpecializationInfo = nullptr;

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].pNext = nullptr;
	stages[1].flags = 0;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = frag_shader;
	stages[1].pName = "main";
	stages[1].pSpecializationInfo = nullptr;

	create_info.stageCount = (uint32_t)stages.size();
	create_info.pStages = stages.data();

	// -- pVertexInputState --

	VkVertexInputBindingDescription vertex_binding_description = {};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = (uint32_t)sizeof(vertex_pos_color_s);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertex_attribute_descriptions;

	// see vertex shader
	vertex_attribute_descriptions[0].location = 0;
	vertex_attribute_descriptions[0].binding = 0;
	vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[0].offset = GET_FIELD_OFFSET(vertex_pos_color_s, pos_);

	vertex_attribute_descriptions[1].location = 1;
	vertex_attribute_descriptions[1].binding = 0;
	vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_attribute_descriptions[1].offset = GET_FIELD_OFFSET(vertex_pos_color_s, color_);

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertex_binding_description,
		.vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descriptions.size(),
		.pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
	};

	create_info.pVertexInputState = &vertex_input_state;

	// -- pInputAssemblyState --

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	create_info.pInputAssemblyState = &input_assembly_state;

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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,	// no cull
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
	vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
	vkDestroyShaderModule(vk_device_, frag_shader, nullptr);

	return rt == VK_SUCCESS;
}

void TriangleDemo::UpdateUniformBuffer() {
	ubo_mvp_s ubo_mvp = {};

	glm::mat4 proj_mat = glm::perspective(glm::radians(70.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 1.0f, 16.0f);

	glm::mat4 view_mat, model_mat;
	GetViewMatrix(view_mat);
	GetModelMatrix(model_mat);

	ubo_mvp.mvp_ = proj_mat * view_mat * model_mat;

	UpdateBuffer(uniform_buffer_mvp_, &ubo_mvp, sizeof(ubo_mvp));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(TriangleDemo)
