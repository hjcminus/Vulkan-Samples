/******************************************************************************
 cubemaps
 *****************************************************************************/

#include "../common/inc.h"
#include "cubemaps.h"

/*
================================================================================
CubeMapsDemo
================================================================================
*/

CubeMapsDemo::CubeMapsDemo():
	vk_sampler_scene_(VK_NULL_HANDLE),
	model_(this),
	vk_desc_set_layout_mvp_cubemap_(VK_NULL_HANDLE),
	vk_desc_set_mvp_cubemap_(VK_NULL_HANDLE),
	vk_pipeline_layout_skybox_(VK_NULL_HANDLE),
	vk_pipeline_layout_model_(VK_NULL_HANDLE),
	vk_pipeline_skybox_(VK_NULL_HANDLE),
	vk_pipeline_model_(VK_NULL_HANDLE)
{
	memset(&tex_cubemap_, 0, sizeof(tex_cubemap_));

	memset(&point_light_, 0, sizeof(point_light_));
	
	// ubo
	memset(&ubo_mvp_, 0, sizeof(ubo_mvp_));
	memset(&ubo_light_, 0, sizeof(ubo_light_));
	memset(&ubo_viewer_, 0, sizeof(ubo_viewer_));

	// vbo
	memset(&vbo_box_, 0, sizeof(vbo_box_));

}

CubeMapsDemo::~CubeMapsDemo() {
	//
}

bool CubeMapsDemo::Init() {
	if (!VkDemo::Init("cubemaps" /* shader files directory */,
		64, 0, 16, 64)) {
		return false;
	}

	if (!CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, vk_sampler_scene_)) {
		return false;
	}

	// "blue_cube.png"
	if (!LoadCubeMaps("blue_cube.png", VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
		vk_sampler_scene_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex_cubemap_)) {
		return false;
	}

	if (!CreateBox()) {
		return false;
	}

	if (!LoadModel()) {
		return false;
	}

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!CreateDescSetLayouts()) {
		return false;
	}

	if (!AllocDescriptorSets()) {
		return false;
	}

	if (!CreatePipelineLayouts()) {
		return false;
	}

	if (!CreatePipelines()) {
		return false;
	}

	// init camera
	camera_mode_ = camera_mode_t::CM_ROTATE_OBJECT;
	camera_rotation_flags_ = ROTATION_YAW_BIT | ROTATION_PITCH_BIT;

	/*
		  y
	 z   /
	 |  /
	 | /
	 |/______x

	 */

	 // z axis is up
	camera_.pos_ = glm::vec3(0.0f, -4.0f, 0.0f);
	camera_.target_ = glm::vec3(0.0f, 0.0f, 0.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	z_near_ = 1.0f;
	z_far_ = 128.0f;

	point_light_.pos_ = glm::vec4(16.0f, -16.0f, 16.0f, 1.0f);
	point_light_.color_ = glm::vec4(1.0f);
	point_light_.radius_ = 64.0f;	// not used

	SetupLight();

	BuildCommandBuffers();

	enable_display_ = true;

	return true;
}

void CubeMapsDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_model_);
	DestroyPipeline(vk_pipeline_skybox_);

	DestroyPipelineLayout(vk_pipeline_layout_model_);
	DestroyPipelineLayout(vk_pipeline_layout_skybox_);

	FreeDescriptorSets();
	DestroyDescriptorSetLayout(vk_desc_set_layout_mvp_cubemap_);

	DestroyUniformBuffers();

	FreeModel();

	DestroyBox();

	DestroyImage(tex_cubemap_);

	DestroySampler(vk_sampler_scene_);

	VkDemo::Shutdown();
}

void CubeMapsDemo::BuildCommandBuffers() {
	uint32_t sz_draw_cmd_buffer = (uint32_t)vk_draw_cmd_buffer_count_;
	for (uint32_t i = 0; i < sz_draw_cmd_buffer; ++i) {
		VkCommandBuffer cmd_buf = vk_draw_cmd_buffers_[i];

		VkCommandBufferBeginInfo cmd_buf_begin_info = {};

		cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buf_begin_info.pNext = nullptr;
		cmd_buf_begin_info.flags = 0;
		cmd_buf_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);

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
		render_pass_begin_info.framebuffer = vk_framebuffers_[i];
		render_pass_begin_info.renderArea.offset.x = 0;
		render_pass_begin_info.renderArea.offset.y = 0;
		render_pass_begin_info.renderArea.extent.width = cfg_viewport_cx_;
		render_pass_begin_info.renderArea.extent.height = cfg_viewport_cy_;

		render_pass_begin_info.clearValueCount = 2; // color, depth-stencil
		render_pass_begin_info.pClearValues = clear_values;

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

		{	// skybox

			VkDescriptorSet descriptor_sets[1] = { vk_desc_set_mvp_cubemap_ };

			vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				vk_pipeline_layout_skybox_, 0, 1, descriptor_sets, 0, nullptr);

			vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_skybox_);

			VkDeviceSize offset[1] = { 0 };
			vkCmdBindVertexBuffers(cmd_buf,
				0, // index of the first vertex input binding
				1, &vbo_box_.buffer_, offset);

			vkCmdDraw(cmd_buf,
				6 * 6,	//  vertexCount
				1,	//	instanceCount: is the number of instances to draw
				0,	//	firstVertex
				0);	//	firstInstance

		}

		{
			// model
			VkDescriptorSet descriptor_sets[1] = { vk_desc_set_mvp_cubemap_ };

			vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				vk_pipeline_layout_model_, 0, 1, descriptor_sets, 0, nullptr);

			vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_model_);

			VkDeviceSize offset[1] = { 0 };
			VkBuffer vertex_buffer = model_.GetVertexBuffer();
			vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer, offset);

			VkBuffer index_buffer = model_.GetIndexBuffer();
			vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd_buf,
				model_.GetIndexCount(),	// indexCount
				1,	// instanceCount
				0, 0, 0);
		}

		vkCmdEndRenderPass(cmd_buf);

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void CubeMapsDemo::WindowSizeChanged() {
	//
}

void CubeMapsDemo::Update() {
	{
		ubo_mvp_sep_s ubo_mvp = {};

		GetProjMatrix(ubo_mvp.proj_);
		GetViewMatrix(ubo_mvp.view_);
		GetModelMatrix(ubo_mvp.model_);

		UpdateBuffer(ubo_mvp_, &ubo_mvp, sizeof(ubo_mvp));
	}

	{
		ubo_viewer_s ubo_viewer = {};

		ubo_viewer.pos_ = glm::vec4(camera_.pos_, 1.0f);

		UpdateBuffer(ubo_viewer_, &ubo_viewer, sizeof(ubo_viewer));
	}
}

bool CubeMapsDemo::CreateBox() {
	const float SCALE = 64.0f;

	std::vector<vertex_pos_s> vertex_buffer = {
		// front face
		{ { -1, 1, -1 } },
		{ { 1, 1, -1 } },
		{ { 1, 1, 1 } },

		{ { 1, 1, 1 } },
		{ { -1, 1, 1 } },
		{ { -1, 1, -1 } },

		// back face
		{ { 1, -1, -1 } },
		{ { -1, -1, -1 } },
		{ { -1, -1, 1 } },

		{ { -1, -1, 1 } },
		{ { 1, -1, 1 } },
		{ { 1, -1, -1 } },

		// left face
		{ { -1, -1, -1 } },
		{ { -1, 1, -1 } },
		{ { -1, 1, 1 } },

		{ { -1, 1, 1 } },
		{ { -1, -1, 1 } },
		{ { -1, -1, -1 } },

		// right face
		{ { 1, 1, -1 } },
		{ { 1, -1, -1 } },
		{ { 1, -1, 1 } },

		{ { 1, -1, 1 } },
		{ { 1, 1, 1 } },
		{ { 1, 1, -1 } },

		// bottom face
		{ { -1, -1, -1 } },
		{ { 1, -1, -1 } },
		{ { 1, 1, -1 } },

		{ { 1, 1, -1 } },
		{ { -1, 1, -1 } },
		{ { -1, -1, -1 } },

		// top face
		{ { -1, 1, 1 } },
		{ { 1, 1, 1 } },
		{ { 1, -1, 1 } },

		{ { 1, -1, 1 } },
		{ { -1, -1, 1 } },
		{ { -1, 1, 1 } },
	};

	for (auto& it : vertex_buffer) {
		it.pos_ *= SCALE;
	}

	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(vertex_pos_s);

	return CreateVertexBuffer(vbo_box_, vertex_buffer.data(), vertex_buffer_size, true);
}

void CubeMapsDemo::DestroyBox() {
	DestroyBuffer(vbo_box_);
}

bool CubeMapsDemo::LoadModel() {
	VkModel::load_params_s  load_params = {
	.desc_set_layout_bind0_mat_bind1_tex_ = VK_NULL_HANDLE
	};

	const char* FILENAME = "sphere.ply";
\
	return model_.Load(load_params, FILENAME, true);
}

void CubeMapsDemo::FreeModel() {
	model_.Free();
}

// uniform buffer
bool CubeMapsDemo::CreateUniformBuffers() {
	return CreateBuffer(ubo_mvp_,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ubo_mvp_sep_s)) &&
		CreateBuffer(ubo_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_point_light_s)) &&
		CreateBuffer(ubo_viewer_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_viewer_s));
}

void CubeMapsDemo::DestroyUniformBuffers() {
	DestroyBuffer(ubo_viewer_);
	DestroyBuffer(ubo_light_);
	DestroyBuffer(ubo_mvp_);
}

bool CubeMapsDemo::CreateDescSetLayouts() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 2, VK_SHADER_STAGE_FRAGMENT_BIT);	// light
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 3, VK_SHADER_STAGE_FRAGMENT_BIT);	// viewer

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_mvp_cubemap_);
}

bool CubeMapsDemo::AllocDescriptorSets() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	std::array<VkDescriptorSetLayout, 1> set_layouts = {
		vk_desc_set_layout_mvp_cubemap_
	};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = (uint32_t)set_layouts.size();
	allocate_info.pSetLayouts = set_layouts.data();

	VkDescriptorSet sets[8] = {};

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, sets)) {
		return false;
	}

	vk_desc_set_mvp_cubemap_ = sets[0];

	update_desc_sets_buffer_s buffer;

	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_cubemap_, 0, ubo_mvp_.buffer_, 0, ubo_mvp_.memory_size_);
	Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_mvp_cubemap_, 1, tex_cubemap_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_cubemap_, 2, ubo_light_.buffer_, 0, ubo_light_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_cubemap_, 3, ubo_viewer_.buffer_, 0, ubo_viewer_.memory_size_);
	
	vkUpdateDescriptorSets(vk_device_,
		(uint32_t)buffer.write_descriptor_sets_.size(), buffer.write_descriptor_sets_.data(), 0, nullptr);

	return true;
}

void CubeMapsDemo::FreeDescriptorSets() {
	if (vk_desc_set_mvp_cubemap_) {
		std::vector<VkDescriptorSet> sets;
		sets.push_back(vk_desc_set_mvp_cubemap_);

		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, (uint32_t)sets.size(), sets.data());
		vk_desc_set_mvp_cubemap_ = VK_NULL_HANDLE;
	}
}

bool CubeMapsDemo::CreatePipelineLayouts() {
	{
		VkPipelineLayoutCreateInfo create_info = {};

		std::array<VkDescriptorSetLayout, 1> descriptorset_layouts = {
			vk_desc_set_layout_mvp_cubemap_
		};

		create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		create_info.pNext = nullptr;
		create_info.flags = 0;
		create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
		create_info.pSetLayouts = descriptorset_layouts.data();
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(vk_device_, &create_info,
			nullptr, &vk_pipeline_layout_skybox_) != VK_SUCCESS) 
		{
			return false;
		}
	}

	{
		VkPipelineLayoutCreateInfo create_info = {};

		std::array<VkDescriptorSetLayout, 1> descriptorset_layouts = {
			vk_desc_set_layout_mvp_cubemap_
		};

		create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		create_info.pNext = nullptr;
		create_info.flags = 0;
		create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
		create_info.pSetLayouts = descriptorset_layouts.data();
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(vk_device_, &create_info,
			nullptr, &vk_pipeline_layout_model_) != VK_SUCCESS)
		{
			return false;
		}
	}

	return true;
}

bool CubeMapsDemo::CreatePipelines() {
	{
		create_pipeline_vert_frag_params_s params = {
			.vertex_shader_filename_ = "SPIR-V/skybox.vert.spv",
			.framgment_shader_filename_ = "SPIR-V/skybox.frag.spv",
			.vertex_format_ = vertex_format_t::VF_POS,
			.instance_format_ = instance_format_t::INST_NONE,
			.additional_vertex_fields_ = 0,
			.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitive_restart_enable_ = VK_FALSE,
			.polygon_mode_ = VK_POLYGON_MODE_FILL,
			.cull_mode_ = VK_CULL_MODE_BACK_BIT,
			.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depth_test_enable_ = VK_TRUE,
			.depth_write_enable_ = VK_TRUE,
			.pipeline_layout_ = vk_pipeline_layout_skybox_,
			.render_pass_ = vk_render_pass_
		};

		if (!CreatePipelineVertFrag(params, vk_pipeline_skybox_)) {
			return false;
		}

	}

	{
		create_pipeline_vert_frag_params_s params = {
			.vertex_shader_filename_ = "SPIR-V/reflect.vert.spv",
			.framgment_shader_filename_ = "SPIR-V/reflect.frag.spv",
			.vertex_format_ = model_.GetVertexFormat(),
			.instance_format_ = instance_format_t::INST_NONE,
			.additional_vertex_fields_ = VERTEX_FIELD_NORMAL,
			.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitive_restart_enable_ = VK_FALSE,
			.polygon_mode_ = VK_POLYGON_MODE_FILL,
			.cull_mode_ = VK_CULL_MODE_BACK_BIT,
			.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depth_test_enable_ = VK_TRUE,
			.depth_write_enable_ = VK_TRUE,
			.pipeline_layout_ = vk_pipeline_layout_model_,
			.render_pass_ = vk_render_pass_
		};

		if (!CreatePipelineVertFrag(params, vk_pipeline_model_)) {
			return false;
		}

	}

	return true;
}

void CubeMapsDemo::SetupLight() {
	UpdateBuffer(ubo_light_, &point_light_, sizeof(point_light_));
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(CubeMapsDemo)
