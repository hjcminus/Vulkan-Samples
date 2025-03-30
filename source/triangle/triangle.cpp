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
	memset(&vertex_buffer_, 0, sizeof(vertex_buffer_));
}

TriangleDemo::~TriangleDemo() {
	// do nothing
}

bool TriangleDemo::Init() {
	if (!VkDemo::Init("triangle" /* shader files directory */,
		1, 0, 0, 1)) {
		return false;
	}

	if (!CreateUniformBuffer()) {
		return false;
	}

	if (!CreateTriangle()) {
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
	camera_rotation_flags_ = ROTATION_YAW_BIT;

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
	DestroyTriangle();
	DestroyUniformBuffer();

	VkDemo::Shutdown();
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
		vkCmdBindVertexBuffers(cmd_buf, 
			0, // index of the first vertex input binding
			1, &vertex_buffer_.buffer_, offset);


		/* When the command is executed, primitives are assembled using the current
		   primitive topology and vertexCount consecutive vertex indices with the
		   first vertexIndex value equal to firstVertex. The primitives are drawn
		   instanceCount times with instanceIndex starting with firstInstance and
		   increasing sequentially for each instance. The assembled primitives execute
		   the bound graphics pipeline.
		 */
		vkCmdDraw(cmd_buf,
			6,	//  vertexCount
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

void TriangleDemo::Update() {
	UpdateUniformBuffer();
}

// uniform buffer
bool TriangleDemo::CreateUniformBuffer() {
	return CreateBuffer(uniform_buffer_mvp_, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ubo_mat_s));
}

void TriangleDemo::DestroyUniformBuffer() {
	DestroyBuffer(uniform_buffer_mvp_);
}

// vertex buffer

bool TriangleDemo::CreateTriangle() {
	std::vector<vertex_pos_color_s> vertex_buffer = {
		{ { -1.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  0.0f, 0.0f,  1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ {  0.0f,-1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
		{ {  0.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
		{ {  0.0f, 0.0f,  1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
	};
	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(vertex_pos_color_s);

	return CreateVertexBuffer(vertex_buffer_, vertex_buffer.data(), vertex_buffer_size, true);
}

void TriangleDemo::DestroyTriangle() {
	DestroyBuffer(vertex_buffer_);
}

// descriptor set layout
bool TriangleDemo::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

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

	std::vector<VkWriteDescriptorSet> write_descriptor_sets;

	Vk_PushWriteDescriptorSet_UBO(write_descriptor_sets, vk_descriptorset_,
		0, uniform_buffer_mvp_);

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
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/color.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/color.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_COLOR,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_ALL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_NONE,	// VK_CULL_MODE_NONE, VK_CULL_MODE_BACK_BIT
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_,
		.render_pass_ = vk_render_pass_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_);
}

void TriangleDemo::UpdateUniformBuffer() {
	ubo_mat_s ubo_mvp = {};

	glm::mat4 proj_mat = glm::perspectiveRH_ZO(glm::radians(45.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 1.0f, 4.0f);

	// revert y // revert y by set VkViewport
	// proj_mat[1][1] *= -1.0f;

	glm::mat4 view_mat, model_mat;
	GetViewMatrix(view_mat);

	GetModelMatrix(model_mat);

	ubo_mvp.matrix_ = proj_mat * view_mat * model_mat;

	UpdateBuffer(uniform_buffer_mvp_, &ubo_mvp, sizeof(ubo_mvp));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(TriangleDemo)
