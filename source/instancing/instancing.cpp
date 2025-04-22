/******************************************************************************
 instancing
 *****************************************************************************/

#include "../common/inc.h"
#include "instancing.h"

/*
================================================================================
InstancingDemo
================================================================================
*/

InstancingDemo::InstancingDemo():
	model_(this),
	vk_desc_set_layout_(VK_NULL_HANDLE),
	vk_desc_set_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_(VK_NULL_HANDLE)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Instance");
#endif

	z_near_ = 1.0f;
	z_far_ = 1024.0f;

	memset(&ubo_mvp_, 0, sizeof(ubo_mvp_));
	memset(&ubo_viewer_, 0, sizeof(ubo_viewer_));
	memset(&ubo_light_, 0, sizeof(ubo_light_));

	memset(&buf_instance_, 0, sizeof(buf_instance_));
}

InstancingDemo::~InstancingDemo() {
	// do nothing
}

bool InstancingDemo::Init() {
	if (!VkDemo::Init("instancing" /* shader files directory */,
		16, 0, 16, 16)) {
		return false;
	}

	SRand(1);

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!LoadModel()) {
		return false;
	}

	if (!CreateInstanceBuffer()) {
		return false;
	}

	if (!CreateDescSetLayout()) {
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

	// init camera
	camera_mode_ = camera_mode_t::CM_ROTATE_OBJECT;

	camera_.pos_ = glm::vec3(0.0f, -384.0f, 0.0f);
	camera_.target_ = glm::vec3(0.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);	// z goes up

	// update viewer ubo after set camera 
	SetupViewerUniformBuffer();
	SetupLightUniformBuffer();

	BuildCommandBuffers();

	enable_display_ = true;

	return true;
}

void InstancingDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_);
	DestroyPipelineLayout(vk_pipeline_layout_);
	FreeDescriptorSets();
	DestroyDescriptorSetLayout(vk_desc_set_layout_);
	DestroyInstanceBuffer();
	FreeModel();
	DestroyUniformBuffers();

	VkDemo::Shutdown();
}

void InstancingDemo::BuildCommandBuffers() {
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

		VkDescriptorSet descriptor_sets[1] = {};
		VkDeviceSize offset[1] = { 0 };

		descriptor_sets[0] = vk_desc_set_;

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, 1, descriptor_sets, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_);
		
		VkBuffer vertex_buffer = model_.GetVertexBuffer();
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer, offset);

		VkBuffer index_buffer = model_.GetIndexBuffer();
		vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		// bind instance buffer
		vkCmdBindVertexBuffers(cmd_buf,
			1, // index of the first vertex input binding
			1,
			&buf_instance_.buffer_, offset);

		// mesh_item_.Cmd_Draw(cmd_buf);
		vkCmdDrawIndexed(cmd_buf,
			model_.GetIndexCount(),	// indexCount
			INSTANCE_COUNT,	// instanceCount
			0, 0, 0);

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void InstancingDemo::Update() {
	UpdateMVPUniformBuffer();
}

// uniform buffer
bool InstancingDemo::CreateUniformBuffers() {
	VkMemoryPropertyFlags mem_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	return CreateBuffer(ubo_mvp_,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		mem_prop_flags,
		sizeof(ubo_mvp_sep_s)) &&
		CreateBuffer(ubo_viewer_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_viewer_s)) &&
		CreateBuffer(ubo_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_directional_light_s));
}

void InstancingDemo::DestroyUniformBuffers() {
	DestroyBuffer(ubo_light_);
	DestroyBuffer(ubo_viewer_);
	DestroyBuffer(ubo_mvp_);
}

bool InstancingDemo::LoadModel() {
	VkModel::load_params_s  load_params = {
		.desc_set_layout_bind0_mat_bind1_tex_ = VK_NULL_HANDLE
	};

	const char* FILENAME = "sphere.ply";	// sphere.ply  bun_zipper.ply

	return model_.Load(load_params, FILENAME, true);
}

void InstancingDemo::FreeModel() {
	model_.Free();
}

bool InstancingDemo::CreateInstanceBuffer() {
	static instance_pos_vec4_s buf[INSTANCE_COUNT];

	glm::vec3 color_table[7] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.5f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f }
	};

	float scale_table[4] = { 1.0f, 2.0f, 4.0f, 6.0f };

	for (int i = 0; i < INSTANCE_COUNT; ++i) {
		// set position
		buf[i].pos_.x = RandNeg1Pos1() * VIEW_DISTANCE;
		buf[i].pos_.y = RandNeg1Pos1() * VIEW_DISTANCE;
		buf[i].pos_.z = RandNeg1Pos1() * VIEW_DISTANCE;

		// set color
		const glm::vec3* color = color_table + (Rand() % 7);

		buf[i].vec4_.r = color->r;
		buf[i].vec4_.g = color->g;
		buf[i].vec4_.b = color->b;

		// set scale
		buf[i].vec4_.w = scale_table[Rand() % 4];
	}

	return CreateVertexBuffer(buf_instance_, buf, sizeof(buf), true);
}

void InstancingDemo::DestroyInstanceBuffer() {
	DestroyBuffer(buf_instance_);
}

// descriptor set layout
bool InstancingDemo::CreateDescSetLayout() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 2, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, 
		&create_info, nullptr, &vk_desc_set_layout_);
}

// descriptor set
bool InstancingDemo::AllocDescriptorSets() {
	VkDescriptorSetAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = vk_descriptor_pool_,
		.descriptorSetCount = 1,
		.pSetLayouts = &vk_desc_set_layout_
	};

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, &vk_desc_set_)) {
		return false;
	}

	update_desc_sets_buffer_s buffer;

	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_, 0, ubo_mvp_.buffer_, 0, ubo_mvp_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_, 1, ubo_viewer_.buffer_, 0, ubo_viewer_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_, 2, ubo_light_.buffer_, 0, ubo_light_.memory_size_);

	vkUpdateDescriptorSets(vk_device_, 
		(uint32_t)buffer.write_descriptor_sets_.size(), buffer.write_descriptor_sets_.data(), 0, nullptr);

	return true;
}

void InstancingDemo::FreeDescriptorSets() {
	if (vk_desc_set_) {
		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, 1, &vk_desc_set_);
		vk_desc_set_ = VK_NULL_HANDLE;
	}
}

// pipeline

bool InstancingDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &vk_desc_set_layout_;	// set 0
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info,
		nullptr, &vk_pipeline_layout_);
}

bool InstancingDemo::CreatePipeline() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/instancing.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/lighting.frag.spv",
		.vertex_format_ = model_.GetVertexFormat(),
		.instance_format_ = instance_format_t::INST_POS_VEC4,
		.additional_vertex_fields_ = VERTEX_FIELD_ALL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_,
		.render_pass_ = vk_render_pass_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_);
}

void InstancingDemo::UpdateMVPUniformBuffer() {
	ubo_mvp_sep_s ubo_mvp_sep = {};

	GetProjMatrix(ubo_mvp_sep.proj_);
	GetViewMatrix(ubo_mvp_sep.view_);
	GetModelMatrix(ubo_mvp_sep.model_);

	UpdateBuffer(ubo_mvp_, &ubo_mvp_sep, sizeof(ubo_mvp_sep));
}

void InstancingDemo::SetupViewerUniformBuffer() {
	ubo_viewer_s ubo_viewer = {};

	ubo_viewer.pos_ = glm::vec4(camera_.pos_, 1.0f);
	UpdateBuffer(ubo_viewer_, &ubo_viewer, sizeof(ubo_viewer));
}

void InstancingDemo::SetupLightUniformBuffer() {
	ubo_directional_light_s ubo_light = {};

	//ubo_light.dir_ = glm::vec4(glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)), 0.0);
	ubo_light.dir_ = glm::vec4(glm::normalize(glm::vec3(-1.0, 1.0f, -1.0f)), 0.0f);
	ubo_light.color_ = glm::vec4(1.0f);

	UpdateBuffer(ubo_light_, &ubo_light, sizeof(ubo_light));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(InstancingDemo)
