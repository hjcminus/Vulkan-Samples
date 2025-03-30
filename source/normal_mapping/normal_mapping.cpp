/******************************************************************************
 normal mapping / bump mapping
 *****************************************************************************/

#include "normal_mapping.h"

/*
================================================================================
NormalMappingDemo
================================================================================
*/

NormalMappingDemo::NormalMappingDemo():
	vk_sampler_(VK_NULL_HANDLE),
	index_count_(0),
	vk_descriptorset_layout_uniform_(VK_NULL_HANDLE),
	vk_descriptorset_layout_sampler_(VK_NULL_HANDLE),
	vk_descriptorset_uniform_(VK_NULL_HANDLE),
	vk_descriptorset_sampler_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_tex_(VK_NULL_HANDLE),
	vk_pipeline_flat_(VK_NULL_HANDLE),
	vk_pipeline_normal_mapping_(VK_NULL_HANDLE),
	render_mode_(render_mode_t::RM_TEXTURE)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Normal mapping (F2 to toggle render mode): [Draw texture]");
#endif

	memset(&texture_color_, 0, sizeof(texture_color_));
	memset(&texture_normal_, 0, sizeof(texture_normal_));

	memset(&uniform_buffer_mat_, 0, sizeof(uniform_buffer_mat_));
	memset(&uniform_buffer_point_light_, 0, sizeof(uniform_buffer_point_light_));

	memset(&vertex_buffer_, 0, sizeof(vertex_buffer_));
	memset(&index_buffer_, 0, sizeof(index_buffer_));
}

NormalMappingDemo::~NormalMappingDemo() {
	// do nothing
}

bool NormalMappingDemo::Init() {
	if (!VkDemo::Init("normal_mapping" /* shader files directory */,
		16, 0, 16, 16)) {
		return false;
	}

	if (!CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, vk_sampler_)) {
		return false;
	}

	if (!LoadTextures()) {
		return false;
	}

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!CreateWall()) {
		return false;
	}

	if (!CreateIndexBuffer()) {
		return false;
	}

	if (!CreateDescriptorSetLayout_Uniform()) {
		return false;
	}

	if (!CreateDescriptorSetLayout_Sampler()) {
		return false;
	}

	if (!AllocDescriptorSets()) {
		return false;
	}

	if (!CreatePipelineLayout()) {
		return false;
	}

	if (!CreatePipeline_Tex()) {
		return false;
	}

	if (!CreatePipeline_Flat()) {
		return false;
	}

	if (!CreatePipeline_NormalMapping()) {
		return false;
	}

	// init camera
	camera_mode_ = camera_mode_t::CM_ROTATE_OBJECT;
	camera_rotation_flags_ = ROTATION_YAW_BIT;

	camera_.pos_ = glm::vec3(0.0f, -2.0f, 0.0f);
	camera_.target_ = glm::vec3(0.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	move_speed_ = 0.1f;

	SetupLightUniformBuffer();

	SetRenderMode(render_mode_t::RM_NORMAL_MAPPING);

	enable_display_ = true;

	printf("F2: switch render mode\n");

	return true;
}

void NormalMappingDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_normal_mapping_);
	DestroyPipeline(vk_pipeline_flat_);
	DestroyPipeline(vk_pipeline_tex_);
	DestroyPipelineLayout(vk_pipeline_layout_);
	FreeDescriptorSets();
	DestroyDescriptorSetLayout(vk_descriptorset_layout_sampler_);
	DestroyDescriptorSetLayout(vk_descriptorset_layout_uniform_);
	DestroyIndexBuffer();
	DestroyWall();
	DestroyUniformBuffers();
	FreeTextures();
	DestroySampler(vk_sampler_);

	VkDemo::Shutdown();
}

void NormalMappingDemo::BuildCommandBuffers() {
	switch (render_mode_) {
	case render_mode_t::RM_TEXTURE:
		BuildCommandBuffer(vk_pipeline_tex_);
		SetTitle("Normal mapping (F2 to toggle render mode): [Draw texture]");
		break;
	case render_mode_t::RM_FLAT_LIGHT:
		BuildCommandBuffer(vk_pipeline_flat_);
		SetTitle("Normal mapping (F2 to toggle render mode): [Flat lighting]");
		break;
	case render_mode_t::RM_NORMAL_MAPPING:
		BuildCommandBuffer(vk_pipeline_normal_mapping_);
		SetTitle("Normal mapping (F2 to toggle render mode): [Normal Mapping]");
		break;
	}
}

void NormalMappingDemo::Update() {
	UpdateMVPUniformBuffer();
}

void NormalMappingDemo::SetRenderMode(render_mode_t mode) {
	render_mode_ = mode;
	BuildCommandBuffers();
}

void NormalMappingDemo::FuncKeyDown(uint32_t key) {
	if (key == KEY_F2) {
		switch (render_mode_) {
		case render_mode_t::RM_TEXTURE:
			SetRenderMode(render_mode_t::RM_FLAT_LIGHT);
			break;
		case render_mode_t::RM_FLAT_LIGHT:
			SetRenderMode(render_mode_t::RM_NORMAL_MAPPING);
			break;
		case render_mode_t::RM_NORMAL_MAPPING:
			SetRenderMode(render_mode_t::RM_TEXTURE);
			break;
		}
	}
}

bool NormalMappingDemo::LoadTextures() {
	if (!Load2DTexture("color.bmp", VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, 
		vk_sampler_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_color_)) {
		return false;
	}

	if (!Load2DTexture("normal.bmp", VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, 
		vk_sampler_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_normal_)) {
		return false;
	}

	return true;
}

void NormalMappingDemo::FreeTextures() {
	Destroy2DImage(texture_normal_);
	Destroy2DImage(texture_color_);
}

bool NormalMappingDemo::CreateUniformBuffers() {
	return CreateBuffer(uniform_buffer_mat_, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_mvp_sep_s))
			&& 
			CreateBuffer(uniform_buffer_point_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_point_light_s));
}

void NormalMappingDemo::DestroyUniformBuffers() {
	DestroyBuffer(uniform_buffer_point_light_);
	DestroyBuffer(uniform_buffer_mat_);
}

bool NormalMappingDemo::CreateWall() {
	std::vector<vertex_pos_normal_uv_tangent_s> vertex_buffer = {
		{ { -1.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, 0.0f,  1.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { -1.0f, 0.0f,  1.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
	};
	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(vertex_pos_normal_uv_tangent_s);

	return CreateVertexBuffer(vertex_buffer_,
		vertex_buffer.data(), vertex_buffer_size, true);
}

void NormalMappingDemo::DestroyWall() {
	DestroyBuffer(vertex_buffer_);
}

// index buffer
bool NormalMappingDemo::CreateIndexBuffer() {
	uint32_t index_buffer[6] = { 0, 1, 2, 2, 3, 0 };
	uint32_t index_buffer_size = static_cast<uint32_t>(sizeof(index_buffer));

	if (!CreateBuffer(index_buffer_, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		index_buffer_size)) {
		return false;
	}

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, index_buffer_.memory_, 0, index_buffer_.memory_size_, 0, &data)) {
		return false;
	}

	memcpy(data, index_buffer, index_buffer_size);

	vkUnmapMemory(vk_device_, index_buffer_.memory_);

	index_count_ = 6;

	return true;
}

void NormalMappingDemo::DestroyIndexBuffer() {
	DestroyBuffer(index_buffer_);
}

// descriptor set layout
bool NormalMappingDemo::CreateDescriptorSetLayout_Uniform() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, 
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_uniform_);
}

bool NormalMappingDemo::CreateDescriptorSetLayout_Sampler() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector< VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_sampler_);

}

// descriptor set
bool NormalMappingDemo::AllocDescriptorSets() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	VkDescriptorSetLayout set_layouts[2] = {
		vk_descriptorset_layout_uniform_,
		vk_descriptorset_layout_sampler_
	};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = 2;
	allocate_info.pSetLayouts = set_layouts;

	VkDescriptorSet sets[2] = {};

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, sets)) {
		return false;
	}

	vk_descriptorset_uniform_ = sets[0];
	vk_descriptorset_sampler_ = sets[1];


	std::vector<VkWriteDescriptorSet> write_descriptor_sets;

	Vk_PushWriteDescriptorSet_UBO(write_descriptor_sets, vk_descriptorset_uniform_,
		0, uniform_buffer_mat_);
	Vk_PushWriteDescriptorSet_UBO(write_descriptor_sets, vk_descriptorset_uniform_,
		1, uniform_buffer_point_light_);
	Vk_PushWriteDescriptorSet_Tex(write_descriptor_sets, vk_descriptorset_sampler_,
		0, texture_color_);
	Vk_PushWriteDescriptorSet_Tex(write_descriptor_sets, vk_descriptorset_sampler_,
		1, texture_normal_);

	vkUpdateDescriptorSets(vk_device_, (uint32_t)write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);

	return true;
}

void NormalMappingDemo::FreeDescriptorSets() {
	if (vk_descriptorset_uniform_) {
		VkDescriptorSet sets[2] = {
			vk_descriptorset_uniform_,
			vk_descriptorset_sampler_
		};
		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, 2, sets);
		
		vk_descriptorset_uniform_ = vk_descriptorset_sampler_ = VK_NULL_HANDLE;
	}
}

// pipeline
bool NormalMappingDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	VkDescriptorSetLayout descriptorset_layouts[2] = {
		vk_descriptorset_layout_uniform_,	// order0: set = 0
		vk_descriptorset_layout_sampler_	// order1: set = 1
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

bool NormalMappingDemo::CreatePipeline_Tex() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/tex.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/tex.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL_UV_TANGENT,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_UV,
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

	return CreatePipelineVertFrag(params, vk_pipeline_tex_);
}

bool NormalMappingDemo::CreatePipeline_Flat() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/flat.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/flat.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL_UV_TANGENT,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_NORMAL | VERTEX_FIELD_UV,
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

	return CreatePipelineVertFrag(params, vk_pipeline_flat_);
}

bool NormalMappingDemo::CreatePipeline_NormalMapping() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/normal_mapping.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/normal_mapping.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL_UV_TANGENT,
		.instance_format_ = instance_format_t::INST_NONE,
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

	return CreatePipelineVertFrag(params, vk_pipeline_normal_mapping_);
}

// build command buffer
void NormalMappingDemo::BuildCommandBuffer(VkPipeline pipeline) {
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

		VkDescriptorSet descriptor_sets[2] = { vk_descriptorset_uniform_, vk_descriptorset_sampler_ };

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, 2, descriptor_sets, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offset[1] = { 0 };
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer_.buffer_, offset);

		vkCmdBindIndexBuffer(cmd_buf, index_buffer_.buffer_, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd_buf, index_count_, 1, 0, 0, 1);

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void NormalMappingDemo::UpdateMVPUniformBuffer() {
	ubo_mvp_sep_s ubo_mvp_sep = {};

	ubo_mvp_sep.proj_ = glm::perspectiveRH_ZO(glm::radians(70.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 0.1f, 16.0f);

	GetViewMatrix(ubo_mvp_sep.view_);
	GetModelMatrix(ubo_mvp_sep.model_);

	UpdateBuffer(uniform_buffer_mat_, &ubo_mvp_sep, sizeof(ubo_mvp_sep));
}

void NormalMappingDemo::SetupLightUniformBuffer() {
	ubo_point_light_s ubo_point_light = {};

	// point light
	ubo_point_light.pos_ = glm::vec4(2.0f, -2.0f, 2.0f, 1.0f);
	ubo_point_light.color_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	ubo_point_light.radius_ = 10.0f;

	UpdateBuffer(uniform_buffer_point_light_, &ubo_point_light, sizeof(ubo_point_light));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(NormalMappingDemo)
