/******************************************************************************
 shadow map
 *****************************************************************************/

#include "../common/inc.h"
#include "shadow_map.h"

/*
================================================================================
ShadowMapDemo
================================================================================
*/

ShadowMapDemo::ShadowMapDemo() :
	old_view_cx_(0),
	old_view_cy_(0),
	model_floor_(this),
	model_object_(this),
	vk_sampler_depth_(VK_NULL_HANDLE),
	vk_shadow_map_depth_format_(VK_FORMAT_D16_UNORM),
	vk_framebuffer_depth_(VK_NULL_HANDLE),
	vk_render_pass_depth_(VK_NULL_HANDLE),
	vk_desc_set_layout_ubo4_tex_(VK_NULL_HANDLE),
	vk_desc_set_layout_ubo_tex_(VK_NULL_HANDLE),
	vk_desc_set_overlay_(VK_NULL_HANDLE),
	vk_desc_set_mvp_light_(VK_NULL_HANDLE),
	vk_desc_set_mvp_viewer_light_(VK_NULL_HANDLE),
	vk_desc_set_mvp_fixed_light_(VK_NULL_HANDLE),
	vk_desc_set_mvp_viewer_light_fixed_(VK_NULL_HANDLE),
	vk_pipeline_layout_(VK_NULL_HANDLE),
	vk_pipeline_overlay_(VK_NULL_HANDLE),
	vk_pipeline_depth_(VK_NULL_HANDLE),
	vk_pipeline_lighting_mat_(VK_NULL_HANDLE),
	vk_pipeline_lighting_tex_(VK_NULL_HANDLE),
	draw_overlay_(true),
	draw_model_(true)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Shadow Map");
#endif

	z_near_ = 2.0f;
	z_far_ = 32.0f;

	light_dir_ = glm::normalize(glm::vec3(-1.0, 0.0f, -1.0f));

	memset(&vk_image_depth_, 0, sizeof(vk_image_depth_));

	memset(&vk_ubo_mvp_overlay_, 0, sizeof(vk_ubo_mvp_overlay_));
	memset(&vk_ubo_mvp_light_, 0, sizeof(vk_ubo_mvp_light_));
	memset(&vk_ubo_mvp_sep_, 0, sizeof(vk_ubo_mvp_sep_));
	memset(&vk_ubo_mvp_fixed_light_, 0, sizeof(vk_ubo_mvp_fixed_light_));
	memset(&vk_ubo_mvp_sep_fixed_, 0, sizeof(vk_ubo_mvp_sep_fixed_));
	memset(&vk_ubo_viewer_, 0, sizeof(vk_ubo_viewer_));
	memset(&vk_ubo_light_, 0, sizeof(vk_ubo_light_));

	memset(&vk_vbo_overlay_, 0, sizeof(vk_vbo_overlay_));
	
}

ShadowMapDemo::~ShadowMapDemo() {
	// do nothing
}

bool ShadowMapDemo::Init() {
	if (!VkDemo::Init("shadow_map" /* shader files directory */,
		16, 0, 16, 16)) {
		return false;
	}

	vk_shadow_map_depth_format_ = GetIdealDepthFormat();

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!CreateSampler_Depth()) {
		return false;
	}

	if (!CreateRenderPass_Depth()) {
		return false;
	}

	if (!CreateDepthImage()) {
		return false;
	}

	if (!CreateDepthFramebuffer()) {
		return false;
	}

	if (!CreateDescriptorSetLayout_UBO4_Tex()) {
		return false;
	}

	if (!CreateDescriptorSetLayout_UBO_Tex()) {
		return false;
	}

	if (!AllocateDestriptorSets()) {
		return false;
	}

	VkModel::load_params_s load_params = {
		.desc_set_layout_bind0_mat_bind1_tex_ = vk_desc_set_layout_ubo_tex_
	};

	if (!model_floor_.Load(load_params, "floor/floor.obj", false)) {
		return false;
	}

	if (model_floor_.GetVertexFormat() != MODEL_EXPECT_VF) {
		printf("Bad vertex format of model_floor\n");
		return false;
	}

	glm::mat4 trans = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
	trans = glm::rotate(trans, glm::radians(90.0f),
		glm::vec3(1.0f, 0.0f, 0.0f));
	if (!model_object_.Load(load_params, "low_poly_tree/Lowpoly_tree_sample.obj", false, &trans)) {
		return false;
	}

	if (model_object_.GetVertexFormat() != MODEL_EXPECT_VF) {
		printf("Bad vertex format of model_object\n");
		return false;
	}

	if (!CreateOverlayVBO()) {
		return false;
	}

	if (!CreatePipelineLayout()) {
		return false;
	}

	if (!CreatePipeline_Overlay()) {
		return false;
	}

	if (!CreatePipeline_Depth()) {
		return false;
	}

	if (!CreatePipeline_Lighting_Mat()) {
		return false;
	}

	if (!CreatePipeline_Lighting_Tex()) {
		return false;
	}

	// init camera
	camera_mode_ = camera_mode_t::CM_ROTATE_OBJECT;
	camera_rotation_flags_ = ROTATION_YAW_BIT;

	camera_.pos_ = glm::vec3(0.0f, -13.0f, 3.0f);
	camera_.target_ = glm::vec3(0.0f, 0.0f, 3.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);	// z goes up

	SetupUBOs();

	UpdateBuffersForOverlay();

	BuildCommandBuffers();

	enable_display_ = true;

	printf("F2: toggle draw overlay\n");

	return true;
}

void ShadowMapDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_lighting_tex_);
	DestroyPipeline(vk_pipeline_lighting_mat_);
	DestroyPipeline(vk_pipeline_depth_);
	DestroyPipeline(vk_pipeline_overlay_);
	DestroyPipelineLayout(vk_pipeline_layout_);

	DestroyBuffer(vk_vbo_overlay_);

	model_object_.Free();
	model_floor_.Free();

	FreeDescriptorSets();

	DestroyDescriptorSetLayout(vk_desc_set_layout_ubo_tex_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_ubo4_tex_);
	DestroyFramebuffer(vk_framebuffer_depth_);
	Destroy2DImage(vk_image_depth_);
	DestroyRenderPass(vk_render_pass_depth_);
	DestroySampler(vk_sampler_depth_);

	DestroyBuffer(vk_vbo_overlay_);

	DestroyBuffer(vk_ubo_light_);
	DestroyBuffer(vk_ubo_viewer_);
	DestroyBuffer(vk_ubo_mvp_sep_fixed_);
	DestroyBuffer(vk_ubo_mvp_fixed_light_);
	DestroyBuffer(vk_ubo_mvp_sep_);
	DestroyBuffer(vk_ubo_mvp_light_);
	DestroyBuffer(vk_ubo_mvp_overlay_);
	
	VkDemo::Shutdown();
}

void ShadowMapDemo::BuildCommandBuffers() {
	const VkModel * models[2] = {&model_floor_, &model_object_};
	const uint32_t draw_model_count = 2; // 1 2

	auto DrawModels = [&](VkCommandBuffer cmd_buf, bool depth_pass, 
		const VkModel * const * models, uint32_t models_count) {

		VkDeviceSize offset[1] = { 0 };

		for (uint32_t i = 0; i < models_count; ++i) {
			const VkModel* m = models[i];

			VkBuffer vertex_buffer = m->GetVertexBuffer();
			vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer, offset);

			VkBuffer index_buffer = m->GetIndexBuffer();
			vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

			if (depth_pass) {
				std::array<VkDescriptorSet, 1> descriptor_sets;

				if (m == &model_floor_) {
					descriptor_sets[0] = vk_desc_set_mvp_fixed_light_;
				}
				else {
					descriptor_sets[0] = vk_desc_set_mvp_light_;
				}

				vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
					vk_pipeline_layout_, 0, (uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

				vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_depth_);

				uint32_t model_part_count = m->GetModelPartCount();
				for (uint32_t k = 0; k < model_part_count; ++k) {
					const VkModel::vk_model_part_s* model_part = m->GetModelPartByIdx(k);
					vkCmdDrawIndexed(cmd_buf, model_part->index_count_, 1, model_part->index_offset_, 0, 0);
				}
			}
			else {
				uint32_t material_count = m->GetMaterialCount();

				for (uint32_t j = 0; j < material_count; ++j) {
					const VkModel::vk_material_s* vk_mat = m->GetMaterialByIdx(j);

					std::array<VkDescriptorSet, 2> descriptor_sets = {
						m == &model_floor_ ? vk_desc_set_mvp_viewer_light_fixed_ : vk_desc_set_mvp_viewer_light_,
						vk_mat->vk_desc_set_,
					};

					vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
						vk_pipeline_layout_, 0, (uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

					if (vk_mat->vk_texture_.image_) {
						// texture
						vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_lighting_tex_);
					}
					else {
						// non-texture
						vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_lighting_mat_);
					}

					uint32_t model_part_count = m->GetModelPartCount();
					for (uint32_t k = 0; k < model_part_count; ++k) {
						const VkModel::vk_model_part_s* model_part = m->GetModelPartByIdx(k);
						if (model_part->material_idx_ == j) {
							vkCmdDrawIndexed(cmd_buf, model_part->index_count_, 1, model_part->index_offset_, 0, 0);
						}
					}
				}
			}

		}
	};

	auto DrawOverlay = [&](VkCommandBuffer cmd_buf) {
		std::array<VkDescriptorSet, 1> descriptor_sets;

		descriptor_sets[0] = vk_desc_set_overlay_;

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_, 0, (uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_overlay_);


		VkDeviceSize offset[1] = { 0 };
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vk_vbo_overlay_.buffer_, offset);

		vkCmdDraw(cmd_buf, 4, 1, 0, 0);
	};

	uint32_t sz_draw_cmd_buffer = (uint32_t)vk_draw_cmd_buffer_count_;
	for (uint32_t i = 0; i < sz_draw_cmd_buffer; ++i) {
		VkCommandBuffer cmd_buf = vk_draw_cmd_buffers_[i];

		VkCommandBufferBeginInfo cmd_buf_begin_info = {};

		cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buf_begin_info.pNext = nullptr;
		cmd_buf_begin_info.flags = 0;
		cmd_buf_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);

		// depth
		{
			VkRenderPassBeginInfo render_pass_begin_info = {};

			VkClearValue clear_values[1];

			clear_values[0].depthStencil.depth = 1.0f;
			clear_values[0].depthStencil.stencil = 0;

			render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_begin_info.pNext = nullptr;

			render_pass_begin_info.renderPass = vk_render_pass_depth_;
			// render_pass_begin_info.framebuffer
			render_pass_begin_info.renderArea.offset.x = 0;
			render_pass_begin_info.renderArea.offset.y = 0;
			render_pass_begin_info.renderArea.extent.width = FRAMEBUFFER_DIM;
			render_pass_begin_info.renderArea.extent.height = FRAMEBUFFER_DIM;

			render_pass_begin_info.clearValueCount = 1; // depth-stencil
			render_pass_begin_info.pClearValues = clear_values;

			render_pass_begin_info.framebuffer = vk_framebuffer_depth_;

			vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			//*
			VkViewport viewport = {
				.x = 0.0f,
				//.y = (float)FRAMEBUFFER_DIM,
				.y = 0.0f,
				.width = (float)FRAMEBUFFER_DIM,
				//.height = -(float)FRAMEBUFFER_DIM,
				.height = (float)FRAMEBUFFER_DIM,
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};
			vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

			VkRect2D scissor;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = FRAMEBUFFER_DIM;
			scissor.extent.height = FRAMEBUFFER_DIM;

			vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
			//*/

			DrawModels(cmd_buf, true, models, draw_model_count);

			vkCmdEndRenderPass(cmd_buf);

		}

		// scene
		{
			VkRenderPassBeginInfo render_pass_begin_info = {};

			VkClearValue clear_values[2];

			clear_values[0].color.float32[0] = 0.0f;
			clear_values[0].color.float32[1] = 0.0f;
			clear_values[0].color.float32[2] = 1.0f;
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

			if (draw_model_) {
				DrawModels(cmd_buf, false, models, draw_model_count);
			}

			if (draw_overlay_) {
				DrawOverlay(cmd_buf);
			}

			vkCmdEndRenderPass(cmd_buf);
		}

		VkResult rt = vkEndCommandBuffer(cmd_buf);
		if (rt != VK_SUCCESS) {
			printf("vkEndCommandBuffer error\n");
		}
	}
}

void ShadowMapDemo::Update() {
	// light view

	frustum_s frustum = {
		.z_near_ = z_near_,
		.z_far_ = z_far_,
		.fovy_ = fovy_,
		.ratio_ = (float)cfg_viewport_cx_ / cfg_viewport_cy_,
		.view_pos_ = camera_.pos_,
		.view_target_ = camera_.target_,
		.view_up_ = camera_.up_
	};

	glm::vec3 frustum_corners[8];
	CalculateFrustumCorners(frustum, frustum_corners);

	glm::vec3 frustum_center;
	CalculateVec3ArrayCenter(frustum_corners, 8, frustum_center);

	float frustum_radius = CalculateMaxDistToVec3Array(frustum_corners, 8, frustum_center);

	glm::vec3 light_pos = frustum_center;
	glm::vec3 light_target = light_pos + light_dir_;

	glm::mat4 light_view = glm::lookAt(light_pos, light_target, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 light_proj = glm::orthoRH_ZO(
		-frustum_radius, frustum_radius,
		-frustum_radius, frustum_radius,
		-frustum_radius, frustum_radius);

	ubo_mat_s ubo_mvp = {};

	glm::mat4 model;
	GetModelMatrix(model);

	ubo_mvp.matrix_ = light_proj * light_view;
	UpdateBuffer(vk_ubo_mvp_fixed_light_, &ubo_mvp, sizeof(ubo_mvp));

	ubo_mvp.matrix_ = ubo_mvp.matrix_ * model;
	UpdateBuffer(vk_ubo_mvp_light_, &ubo_mvp, sizeof(ubo_mvp));

	// camera view

	ubo_mvp_sep_s ubo_mvp_sep = {};

	GetProjMatrix(ubo_mvp_sep.proj_);
	GetViewMatrix(ubo_mvp_sep.view_);

	ubo_mvp_sep.model_ = glm::mat4(1.0f);

	UpdateBuffer(vk_ubo_mvp_sep_fixed_, &ubo_mvp_sep, sizeof(ubo_mvp_sep));

	ubo_mvp_sep.model_ = model;
	UpdateBuffer(vk_ubo_mvp_sep_, &ubo_mvp_sep, sizeof(ubo_mvp_sep));

	UpdateBuffersForOverlay();
}

void ShadowMapDemo::FuncKeyDown(uint32_t key) {
	if (key == KEY_F2) {
		draw_overlay_ = !draw_overlay_;
		BuildCommandBuffers();
	}
	else if (key == KEY_F3) {
		draw_model_ = !draw_model_;
		BuildCommandBuffers();
	}
}

bool ShadowMapDemo::CreateUniformBuffers() {
	VkMemoryPropertyFlags mem_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	return 
		CreateBuffer(vk_ubo_mvp_overlay_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_mat_s)) &&
		CreateBuffer(vk_ubo_mvp_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_mat_s)) &&
		CreateBuffer(vk_ubo_mvp_sep_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_mvp_sep_s)) &&
		CreateBuffer(vk_ubo_mvp_fixed_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_mat_s)) &&
		CreateBuffer(vk_ubo_mvp_sep_fixed_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_mvp_sep_s)) &&
		CreateBuffer(vk_ubo_viewer_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_viewer_s)) &&
		CreateBuffer(vk_ubo_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			mem_prop_flags,
			sizeof(ubo_directional_light_s));
}

bool ShadowMapDemo::CreateSampler_Depth() {
	return CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk_sampler_depth_);
}

bool ShadowMapDemo::CreateRenderPass_Depth() {
	VkRenderPassCreateInfo create_info = {};

	std::array<VkAttachmentDescription, 1> attachment_descs;

	attachment_descs[0].flags = 0;
	attachment_descs[0].format = vk_shadow_map_depth_format_;
	attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	std::array<VkSubpassDescription, 1> subpass_descs;

	VkAttachmentReference depth_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	subpass_descs[0].flags = 0;
	subpass_descs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_descs[0].inputAttachmentCount = 0;
	subpass_descs[0].pInputAttachments = nullptr;
	subpass_descs[0].colorAttachmentCount = 0;
	subpass_descs[0].pColorAttachments = nullptr;
	subpass_descs[0].pResolveAttachments = nullptr;
	subpass_descs[0].pDepthStencilAttachment = &depth_reference;
	subpass_descs[0].preserveAttachmentCount = 0;
	subpass_descs[0].pPreserveAttachments = nullptr;

	// subpass dependency
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.attachmentCount = (uint32_t)attachment_descs.size();
	create_info.pAttachments = attachment_descs.data();
	create_info.subpassCount = (uint32_t)subpass_descs.size();
	create_info.pSubpasses = subpass_descs.data();
	create_info.dependencyCount = (uint32_t)dependencies.size();
	create_info.pDependencies = dependencies.data();

	return VK_SUCCESS == vkCreateRenderPass(vk_device_, &create_info, nullptr, &vk_render_pass_depth_);
}

bool ShadowMapDemo::CreateDepthImage() {
	return Create2DImage(vk_image_depth_, vk_shadow_map_depth_format_, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		FRAMEBUFFER_DIM, FRAMEBUFFER_DIM, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, vk_sampler_depth_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

bool ShadowMapDemo::CreateDepthFramebuffer() {
	VkFramebufferCreateInfo create_info = {};

	std::array<VkImageView, 1> attachments = {
		vk_image_depth_.image_view_
	};

	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.renderPass = vk_render_pass_depth_;
	create_info.attachmentCount = (uint32_t)attachments.size();
	create_info.pAttachments = attachments.data();
	create_info.width = FRAMEBUFFER_DIM;
	create_info.height = FRAMEBUFFER_DIM;
	create_info.layers = 1;

	return VK_SUCCESS == vkCreateFramebuffer(vk_device_, &create_info, nullptr, &vk_framebuffer_depth_);
}

bool ShadowMapDemo::CreateDescriptorSetLayout_UBO4_Tex() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 1, VK_SHADER_STAGE_VERTEX_BIT);	// light mvp
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 2, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 3, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);	// light
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 4, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_ubo4_tex_);
}

bool ShadowMapDemo::CreateDescriptorSetLayout_UBO_Tex() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_ubo_tex_);
}

static const int NUM_DESC_SETS = 5;

bool ShadowMapDemo::AllocateDestriptorSets() {
	VkDescriptorSetAllocateInfo allocate_info = {};

	std::array<VkDescriptorSetLayout, NUM_DESC_SETS> set_layouts = {
		vk_desc_set_layout_ubo4_tex_,
		vk_desc_set_layout_ubo4_tex_,
		vk_desc_set_layout_ubo4_tex_,
		vk_desc_set_layout_ubo4_tex_,
		vk_desc_set_layout_ubo4_tex_
	};

	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = vk_descriptor_pool_;
	allocate_info.descriptorSetCount = (uint32_t)set_layouts.size();
	allocate_info.pSetLayouts = set_layouts.data();

	VkDescriptorSet sets[NUM_DESC_SETS] = {};

	if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, sets)) {
		return false;
	}

	vk_desc_set_overlay_ = sets[0];
	vk_desc_set_mvp_light_ = sets[1];
	vk_desc_set_mvp_viewer_light_ = sets[2];
	vk_desc_set_mvp_fixed_light_ = sets[3];
	vk_desc_set_mvp_viewer_light_fixed_ = sets[4];

	update_desc_sets_buffer_s buffer;

	// overlay
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_overlay_, 0, vk_ubo_mvp_overlay_.buffer_, 0, vk_ubo_mvp_overlay_.memory_size_);
	Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_overlay_, 4, vk_image_depth_);

	// object
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_light_, 0, vk_ubo_mvp_light_.buffer_, 0, vk_ubo_mvp_light_.memory_size_);
	
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_, 0, vk_ubo_mvp_sep_.buffer_, 0, vk_ubo_mvp_sep_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_, 1, vk_ubo_mvp_light_.buffer_, 0, vk_ubo_mvp_light_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_, 2, vk_ubo_viewer_.buffer_, 0, vk_ubo_viewer_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_, 3, vk_ubo_light_.buffer_, 0, vk_ubo_light_.memory_size_);
	Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_mvp_viewer_light_, 4, vk_image_depth_);

	// floor
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_fixed_light_, 0, vk_ubo_mvp_fixed_light_.buffer_, 0, vk_ubo_mvp_fixed_light_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_fixed_, 0, vk_ubo_mvp_sep_fixed_.buffer_, 0, vk_ubo_mvp_sep_fixed_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_fixed_, 1, vk_ubo_mvp_fixed_light_.buffer_, 0, vk_ubo_mvp_fixed_light_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_fixed_, 2, vk_ubo_viewer_.buffer_, 0, vk_ubo_viewer_.memory_size_);
	Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_light_fixed_, 3, vk_ubo_light_.buffer_, 0, vk_ubo_light_.memory_size_);
	Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_mvp_viewer_light_fixed_, 4, vk_image_depth_);

	vkUpdateDescriptorSets(vk_device_,
		(uint32_t)buffer.write_descriptor_sets_.size(), 
		buffer.write_descriptor_sets_.data(), 0, nullptr);

	return true;
}

void ShadowMapDemo::FreeDescriptorSets() {
	if (vk_desc_set_overlay_) {
		std::array<VkDescriptorSet, NUM_DESC_SETS>	desc_sets = {
			vk_desc_set_overlay_,
			vk_desc_set_mvp_light_,
			vk_desc_set_mvp_viewer_light_,
			vk_desc_set_mvp_fixed_light_,
			vk_desc_set_mvp_viewer_light_fixed_
		};

		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, (uint32_t)desc_sets.size(), desc_sets.data());

		vk_desc_set_overlay_ = VK_NULL_HANDLE;
		vk_desc_set_mvp_light_ = VK_NULL_HANDLE;
		vk_desc_set_mvp_viewer_light_ = VK_NULL_HANDLE;
		vk_desc_set_mvp_fixed_light_ = VK_NULL_HANDLE;
		vk_desc_set_mvp_viewer_light_fixed_ = VK_NULL_HANDLE;
	}
}

bool ShadowMapDemo::CreateOverlayVBO() {
	return CreateVertexBuffer(vk_vbo_overlay_, nullptr, sizeof(vertex_pos_uv_s) * 4, false /* no staging */);
}

bool ShadowMapDemo::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayout, 2> descriptorset_layouts = {
		vk_desc_set_layout_ubo4_tex_,	// set = 0
		vk_desc_set_layout_ubo_tex_		// set = 1
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
	create_info.pSetLayouts = descriptorset_layouts.data();
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info,
		nullptr, &vk_pipeline_layout_);
}

bool ShadowMapDemo::CreatePipeline_Overlay() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/overlay.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/overlay.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_UV,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_UV,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_test_enable_ = VK_FALSE,
		.depth_write_enable_ = VK_FALSE,
		.pipeline_layout_ = vk_pipeline_layout_,
		.render_pass_ = vk_render_pass_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_overlay_);
}

bool ShadowMapDemo::CreatePipeline_Depth() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/depth.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/depth.frag.spv",
		.vertex_format_ = MODEL_EXPECT_VF,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = 0,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_NONE,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_,
		.render_pass_ = vk_render_pass_depth_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_depth_);
}

bool ShadowMapDemo::CreatePipeline_Lighting_Mat() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/lighting_mat.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/lighting_mat.frag.spv",
		.vertex_format_ = MODEL_EXPECT_VF,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_NORMAL,
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

	return CreatePipelineVertFrag(params, vk_pipeline_lighting_mat_);
}

bool ShadowMapDemo::CreatePipeline_Lighting_Tex() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/lighting_tex.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/lighting_tex.frag.spv",
		.vertex_format_ = MODEL_EXPECT_VF,
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

	return CreatePipelineVertFrag(params, vk_pipeline_lighting_tex_);
}

void ShadowMapDemo::SetupUBOs() {
	ubo_viewer_s ubo_viewer = {};

	ubo_viewer.pos_ = glm::vec4(camera_.pos_, 1.0f);
	UpdateBuffer(vk_ubo_viewer_, &ubo_viewer, sizeof(ubo_viewer));

	ubo_directional_light_s ubo_light = {};

	ubo_light.dir_ = glm::vec4(light_dir_, 0.0f);
	ubo_light.color_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	UpdateBuffer(vk_ubo_light_, &ubo_light, sizeof(ubo_light));
}

void ShadowMapDemo::UpdateBuffersForOverlay() {

	auto UpdateOverlayVBO = [&]() {
		const float BORDER_WIDTH = 10.0f;
		const float OVERLAY_DIM = 200.0f;

		vertex_pos_uv_s	vertices[4] = {
			{ glm::vec3(BORDER_WIDTH, cfg_viewport_cy_ - BORDER_WIDTH - OVERLAY_DIM, 0.0f), glm::vec2(0.0f, 0.0f) },
			{ glm::vec3(BORDER_WIDTH + OVERLAY_DIM, cfg_viewport_cy_ - BORDER_WIDTH - OVERLAY_DIM, 0.0f), glm::vec2(1.0f, 0.0f) },
			{ glm::vec3(BORDER_WIDTH + OVERLAY_DIM, cfg_viewport_cy_ - BORDER_WIDTH, 0.0f), glm::vec2(1.0f, 1.0f) },
			{ glm::vec3(BORDER_WIDTH, cfg_viewport_cy_ - BORDER_WIDTH, 0.0f), glm::vec2(0.0f, 1.0f) }
		};

		UpdateBuffer(vk_vbo_overlay_, &vertices, sizeof(vertices));
	};

	auto UpateMVPUBO = [&]() {
		ubo_mat_s ubo_mvp = {};

		ubo_mvp.matrix_ = glm::ortho(0.0f, (float)cfg_viewport_cx_, 0.0f, (float)cfg_viewport_cy_);

		UpdateBuffer(vk_ubo_mvp_overlay_, &ubo_mvp, sizeof(ubo_mvp));
	};

	if (cfg_viewport_cx_ != old_view_cx_ || cfg_viewport_cy_ != old_view_cy_) {

		old_view_cx_ = cfg_viewport_cx_;
		old_view_cy_ = cfg_viewport_cy_;

		/*
		
		y
		|_________________
		|  __             |
		| |__| // overlay |
		|    viewport     |
		O_________________|____x

		
		*/

		UpdateOverlayVBO();

		UpateMVPUBO();
	}
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(ShadowMapDemo)
