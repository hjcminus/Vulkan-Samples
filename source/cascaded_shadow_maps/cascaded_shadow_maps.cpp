/******************************************************************************
 cascaded shadow maps
 *****************************************************************************/

#include "../common/inc.h"
#include "cascaded_shadow_maps.h"

/*
================================================================================
CascadedShadowMapsDemo
================================================================================
*/

CascadedShadowMapsDemo::CascadedShadowMapsDemo() :
	vk_sampler_depth_(VK_NULL_HANDLE),
	vk_sampler_scene_(VK_NULL_HANDLE),
	vk_shadow_map_depth_format_(VK_FORMAT_D16_UNORM),
	vk_render_pass_depth_(VK_NULL_HANDLE),
	mvp_buffer_size_(sizeof(glm::mat4)),
	terrain_edge_length_(1.0f),
	tree_(this),
	tree_scale_(1.0f),
	tree_z_delta_(0.0f),
	vk_desc_set_layout_depth_(VK_NULL_HANDLE),
	vk_desc_set_layout_overlay_(VK_NULL_HANDLE),
	vk_desc_set_layout_mvp_viewer_depth_tex_(VK_NULL_HANDLE),
	vk_desc_set_layout_terrain_tex_(VK_NULL_HANDLE),
	vk_desc_set_layout_material_texture_(VK_NULL_HANDLE),
	vk_desc_set_layout_bind0_ubo_(VK_NULL_HANDLE),
	vk_desc_set_layout_terrain_(VK_NULL_HANDLE),
	vk_desc_set_layout_cas_(VK_NULL_HANDLE),
	vk_desc_set_overlay_(VK_NULL_HANDLE),
	vk_desc_set_terrain_tex_(VK_NULL_HANDLE),
	vk_desc_set_terrain_(VK_NULL_HANDLE),
	vk_desc_set_mvp_viewer_depth_tex_(VK_NULL_HANDLE),
	vk_desc_set_light_(VK_NULL_HANDLE),
	vk_desc_set_model_mat_(VK_NULL_HANDLE),
	vk_desc_set_fog_(VK_NULL_HANDLE),
	vk_desc_set_cas_(VK_NULL_HANDLE),
	vk_pipeline_layout_depth_(VK_NULL_HANDLE),
	vk_pipeline_layout_overlay_(VK_NULL_HANDLE),
	vk_pipeline_layout_terrain_(VK_NULL_HANDLE),
	vk_pipeline_layout_vegetation_(VK_NULL_HANDLE),
	vk_pipeline_depth_terrain_(VK_NULL_HANDLE),
	vk_pipeline_depth_vegetation_(VK_NULL_HANDLE),
	vk_pipeline_overlay_(VK_NULL_HANDLE),
	vk_pipeline_terrain_fill_(VK_NULL_HANDLE),
	vk_pipeline_terrain_line_(VK_NULL_HANDLE),
	vk_pipeline_vegetation_mat_fill_(VK_NULL_HANDLE),
	vk_pipeline_vegetation_mat_line_(VK_NULL_HANDLE),
	vk_pipeline_vegetation_tex_fill_(VK_NULL_HANDLE),
	vk_pipeline_vegetation_tex_line_(VK_NULL_HANDLE),
	wireframe_mode_(false),
	draw_vegetation_(true),
	draw_overlay_(false),
	draw_debug_(false),
	draw_fog_(true),
	random_seed_(1),
	light_angle_(45.0f),
	fog_start_(64.0f),
	sky_color_(0.3f, 0.7f, 1.0f, 1.0f)
{
#if defined(_WIN32)
	cfg_demo_win_class_name_ = TEXT("Cascaded Shadow Maps");
#endif

	cfg_viewport_cx_ = 1024;
	cfg_viewport_cy_ = 600;
	z_near_ = 1.0f;
	z_far_ = 128.0f;	// 128 1024

	memset(&ubo_cas_, 0, sizeof(ubo_cas_));

	memset(&shadow_maps_, 0, sizeof(shadow_maps_));

	memset(&texture_terrain_, 0, sizeof(texture_terrain_));
	memset(&texture_detail_, 0, sizeof(texture_detail_));
	memset(&vk_image_depth_, 0, sizeof(vk_image_depth_));

	memset(&uniform_buffer_depth_mvp_list_, 0, sizeof(uniform_buffer_depth_mvp_list_));
	memset(&uniform_buffer_overlay_mvp_, 0, sizeof(uniform_buffer_overlay_mvp_));
	memset(&uniform_buffer_mvp_, 0, sizeof(uniform_buffer_mvp_));
	memset(&uniform_buffer_model_matrix_, 0, sizeof(uniform_buffer_model_matrix_));
	memset(&uniform_buffer_viewer_, 0, sizeof(uniform_buffer_viewer_));
	memset(&uniform_buffer_terrain_, 0, sizeof(uniform_buffer_terrain_));
	memset(&uniform_buffer_directional_light_, 0, sizeof(uniform_buffer_directional_light_));
	memset(&uniform_buffer_fog_, 0, sizeof(uniform_buffer_fog_));
	memset(&uniform_buffer_cas_, 0, sizeof(uniform_buffer_cas_));

	memset(&terrain_, 0, sizeof(terrain_));
	memset(&instance_buffer_, 0, sizeof(instance_buffer_));
	memset(&overlay_vertex_buffer_, 0, sizeof(overlay_vertex_buffer_));
	
	vertex_count_per_edge_ = Terrain_GetVertexCountPerEdge(TERRAIN_SIZE);
	terrain_edge_length_ = (float)(vertex_count_per_edge_ - 1);

	memset(instance_buf_, 0, sizeof(instance_buf_));

	memset(&texture_tiles_, 0, sizeof(texture_tiles_));

	UpdateSkyColor();
}

CascadedShadowMapsDemo::~CascadedShadowMapsDemo() {
	// do nothing
}

bool CascadedShadowMapsDemo::Init() {
	if (!VkDemo::Init("cascaded_shadow_maps" /* shader files directory */,
		64, 0, 16, 64)) {
		return false;
	}

	vk_shadow_map_depth_format_ = GetIdealDepthFormat();
	printf("vk_shadow_map_depth_format_ = %s\n", Vk_FormatToStr(vk_shadow_map_depth_format_));

	mvp_buffer_size_ = (uint32_t)GetAlignedMinOffsetSize(sizeof(ubo_mat_s));

	// init textur tiles
	Str_SPrintf(texture_tiles_.lowest_, MAX_PATH, "%s/terrain/lowestTile.tga", textures_dir_);
	Str_SPrintf(texture_tiles_.low_, MAX_PATH, "%s/terrain/lowTile.tga", textures_dir_);
	Str_SPrintf(texture_tiles_.high_, MAX_PATH, "%s/terrain/HighTile.tga", textures_dir_);
	Str_SPrintf(texture_tiles_.hightest_, MAX_PATH, "%s/terrain/highestTile.tga", textures_dir_);

	if (!CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk_sampler_depth_)) {
		return false;
	}

	if (!CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, vk_sampler_scene_)) {
		return false;
	}

	if (!LoadTextures()) {
		return false;
	}

	if (!CreateRenderPass_Depth()) {
		return false;
	}

	if (!CreateDepthImage()) {
		return false;
	}

	if (!CreateDepthFramebuffers()) {
		return false;
	}

	if (!CreateUniformBuffers()) {
		return false;
	}

	SetupTerrainUniformBuffer();

	if (!CreateTerrainBuffers()) {
		return false;
	}

	if (!CreateInstanceBuffer()) {
		return false;
	}

	if (!CreateOverlayVertexBuffer()) {
		return false;
	}

	UpdateTerrain();

	if (!CreateDescSetLayout_Depth()) {
		return false;
	}

	if (!CreateDescSetLayout_Overlay()) {
		return false;
	}

	if (!CreateDescSetLayout_MVPViewerDepthTex()) {
		return false;
	}

	if (!CreateDescSetLayout_MaterialTexture()) {
		return false;
	}

	if (!CreateDescSetLayout_Bind0_UBO()) {
		return false;
	}

	if (!CreateDescSetLayout_TerrainTex()) {
		return false;
	}

	if (!CreateDescSetLayout_Terrain()) {
		return false;
	}

	if (!CreateDescSetLayout_Cas()) {
		return false;
	}

	if (!AllocDescriptorSets()) {
		return false;
	}

	if (!LoadModel()) {
		return false;
	}

	if (!CreatePipelineLayout_Depth()) {
		return false;
	}

	if (!CreatePipelineLayout_Overlay()) {
		return false;
	}

	if (!CreatePipelineLayout_Terrain()) {
		return false;
	}

	if (!CreatePipelineLayout_Vegetation()) {
		return false;
	}

	if (!CreatePipeline_Overlay()) {
		return false;
	}

	if (!CreatePipeline_Depth_Terrain()) {
		return false;
	}

	if (!CreatePipeline_Depth_Vegetation()) {
		return false;
	}

	if (!CreatePipelines_Terrain()) {
		return false;
	}

	if (!CreatePipelines_Vegetation_Mat()) {
		return false;
	}

	if (!CreatePipelines_Vegetation_Tex()) {
		return false;
	}

	OnViewportChanged();

	SetupModelMatrixUniformBuffer();
	SetupLightUniformBuffer();
	SetupFogUniformBuffer();

	BuildCommandBuffers();

	// init camera
	camera_mode_ = camera_mode_t::CM_MOVE_CAMERA;

	float half_dim = terrain_edge_length_ * 0.5f;

	camera_.pos_ = glm::vec3(half_dim, half_dim, terrain_edge_length_);
	camera_.target_ = glm::vec3(half_dim, half_dim + 1.0f, terrain_edge_length_);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	move_speed_ = 0.5f;

	// calulate cascade split factors

	// copy from vulkanExample, modified
	const float CASCATE_SPLIT_LAMBDA = 0.75f;

	float z_range = z_far_ - z_near_;
	float ratio = z_far_ / z_near_;
	float cur_shadow_map_z_near = z_near_;

	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_COUNT);
		float log = z_near_ * std::pow(ratio, p);
		float uniform = z_near_ + z_range * p;
		float d = CASCATE_SPLIT_LAMBDA * (log - uniform) + uniform;
		float factor = (d - z_near_) / z_range;
		shadow_maps_[i].z_near_ = cur_shadow_map_z_near;
		float z_far = z_near_ + z_range * factor;
		float z_far_expanded = z_far * 1.005f;
		shadow_maps_[i].z_far_ = z_far_expanded < z_far_ ? z_far_expanded : z_far_;
		cur_shadow_map_z_near = z_far;

		// printf("cascde[%d]: z_near_ = %f, z_far_ = %f\n", i, shadow_maps_[i].z_near_, shadow_maps_[i].z_far_);

		ubo_cas_.z_far_[i].x = shadow_maps_[i].z_far_;
	}

	WindowSizeChanged();

	enable_display_ = true;

	printf("F2: switch fill & line mode\n");
	printf("F3: update terrain\n");
	printf("F4: toggle draw vegetation\n");
	printf("F5: toggle draw overlay\n");
	printf("F6: toggle draw debug\n");
	printf("F7: toggle fog\n");
	printf("Page Up/Page Down: change light direction\n");

	return true;
}

void CascadedShadowMapsDemo::Shutdown() {
	DestroyPipeline(vk_pipeline_vegetation_tex_line_);
	DestroyPipeline(vk_pipeline_vegetation_tex_fill_);
	DestroyPipeline(vk_pipeline_vegetation_mat_line_);
	DestroyPipeline(vk_pipeline_vegetation_mat_fill_);
	DestroyPipeline(vk_pipeline_terrain_line_);
	DestroyPipeline(vk_pipeline_terrain_fill_);
	DestroyPipeline(vk_pipeline_overlay_);
	DestroyPipeline(vk_pipeline_depth_vegetation_);
	DestroyPipeline(vk_pipeline_depth_terrain_);
	DestroyPipelineLayout(vk_pipeline_layout_vegetation_);
	DestroyPipelineLayout(vk_pipeline_layout_terrain_);
	DestroyPipelineLayout(vk_pipeline_layout_overlay_);
	DestroyPipelineLayout(vk_pipeline_layout_depth_);
	FreeModel();
	FreeDescriptorSets();
	DestroyDescriptorSetLayout(vk_desc_set_layout_cas_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_terrain_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_bind0_ubo_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_material_texture_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_terrain_tex_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_mvp_viewer_depth_tex_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_overlay_);
	DestroyDescriptorSetLayout(vk_desc_set_layout_depth_);
	DestroyOverlayVertexBuffer();
	DestroyInstanceBuffer();
	DestroyTerrainBuffers();
	DestroyUniformBuffers();

	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
		if (shadow_maps_[i].depth_image_view_) {
			vkDestroyImageView(vk_device_, shadow_maps_[i].depth_image_view_, nullptr);
			shadow_maps_[i].depth_image_view_ = VK_NULL_HANDLE;
		}
		DestroyFramebuffer(shadow_maps_[i].framebuffer_);
	}

	DestroyImage(vk_image_depth_);
	DestroyRenderPass(vk_render_pass_depth_);
	FreeTextures();
	DestroySampler(vk_sampler_scene_);
	DestroySampler(vk_sampler_depth_);

	VkDemo::Shutdown();
}

void CascadedShadowMapsDemo::BuildCommandBuffers() {
	scene_pipelines_s scene_pipelines;

	if (wireframe_mode_) {
		scene_pipelines.pipeline_terrain_ = vk_pipeline_terrain_line_;
		scene_pipelines.pipeline_vegetation_mat_ = vk_pipeline_vegetation_mat_line_;
		scene_pipelines.pipeline_vegetation_tex_ = vk_pipeline_vegetation_tex_line_;
	}
	else {
		scene_pipelines.pipeline_terrain_ = vk_pipeline_terrain_fill_;
		scene_pipelines.pipeline_vegetation_mat_ = vk_pipeline_vegetation_mat_fill_;
		scene_pipelines.pipeline_vegetation_tex_ = vk_pipeline_vegetation_tex_fill_;
	}

	BuildCommandBuffer(scene_pipelines);
}

void CascadedShadowMapsDemo::WindowSizeChanged() {
	OnViewportChanged();
}

void CascadedShadowMapsDemo::Update() {
	UpdateMVPViewerUniformBuffers();

	char* buf = (char*)MapBuffer(uniform_buffer_depth_mvp_list_);
	if (!buf) {
		return;	// error
	}
	char* cur_buf = buf;

	float w_over_h = (float)cfg_viewport_cx_ / cfg_viewport_cy_;

	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; i++) {
		// update lightmap ubo

		frustum_s frustum = {
			.z_near_ = shadow_maps_[i].z_near_,
			.z_far_ = shadow_maps_[i].z_far_,
			.fovy_ = fovy_,
			.ratio_ = w_over_h,
			.view_pos_ = camera_.pos_,
			.view_target_ = camera_.target_,
			.view_up_ = camera_.up_
		};

		glm::vec3 frustum_corners[8];
		CalculateFrustumCorners(frustum, frustum_corners);

		glm::vec3 frustum_center;
		CalculateVec3ArrayCenter(frustum_corners, 8, frustum_center);

		float frustom_radius = shadow_maps_[i].frustum_radius_;

		glm::vec3 light_dir;
		CalcLightDir(light_dir);

		const glm::vec3 LIGHT_RIGHT = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 light_target = frustum_center;
		glm::vec3 light_pos = light_target - light_dir * frustom_radius;
		glm::vec3 light_up = glm::cross(LIGHT_RIGHT, light_dir);

		glm::mat4 light_view_mat = glm::lookAt(light_pos, light_target, light_up);
		
		glm::mat4 light_proj_mat = glm::orthoRH_ZO(
			-frustom_radius, frustom_radius,
			-frustom_radius, frustom_radius,
			0.0f, frustom_radius * 2.0f);

		ubo_mat_s* ubo_mat = (ubo_mat_s*)cur_buf;
		ubo_cas_.light_vp[i] = ubo_mat->matrix_ = light_proj_mat * light_view_mat;

		cur_buf += mvp_buffer_size_;
	}

	UnmapBuffer(uniform_buffer_depth_mvp_list_);

	UpdateBuffer(uniform_buffer_cas_, &ubo_cas_, sizeof(ubo_cas_));
}

void CascadedShadowMapsDemo::FuncKeyDown(uint32_t key) {
	if (key == KEY_F2) {
		wireframe_mode_ = !wireframe_mode_;
		BuildCommandBuffers();
	}
	else if (key == KEY_F3) {
		UpdateTerrain();
	}
	else if (key == KEY_F4) {
		draw_vegetation_ = !draw_vegetation_;
		BuildCommandBuffers();
	}
	else if (key == KEY_F5) {
		draw_overlay_ = !draw_overlay_;
		BuildCommandBuffers();
	}
	else if (key == KEY_F6) {
		draw_debug_ = !draw_debug_;
		BuildCommandBuffers();
	}
	else if (key == KEY_F7) {
		draw_fog_ = !draw_fog_;
		BuildCommandBuffers();
	}
	else if (key == KEY_PAGEDOWN) {
		light_angle_ += 1.0f;
		if (light_angle_ > 180.0f) {
			light_angle_ = 180.0f;
		}
		UpdateSkyColor();
		SetupLightUniformBuffer();
		SetupFogUniformBuffer();

		BuildCommandBuffers();
	}
	else if (key == KEY_PAGEUP) {
		light_angle_ -= 1.0f;
		if (light_angle_ < 0.0f) {
			light_angle_ = 0.0f;
		}
		UpdateSkyColor();
		SetupLightUniformBuffer();
		SetupFogUniformBuffer();

		BuildCommandBuffers();
	}
}

void CascadedShadowMapsDemo::OnViewportChanged() {
	float w_over_h = (float)cfg_viewport_cx_ / cfg_viewport_cy_;

	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; i++) {
		frustum_s frustum = {
			.z_near_ = shadow_maps_[i].z_near_,
			.z_far_ = shadow_maps_[i].z_far_,
			.fovy_ = fovy_,
			.ratio_ = w_over_h,
			.view_pos_ = camera_.pos_,
			.view_target_ = camera_.target_,
			.view_up_ = camera_.up_
		};

		glm::vec3 frustum_corners[8];
		CalculateFrustumCorners(frustum, frustum_corners);

		glm::vec3 center;
		CalculateVec3ArrayCenter(frustum_corners, 8, center);

		shadow_maps_[i].frustum_radius_ = CalculateMaxDistToVec3Array(frustum_corners, 8, center);
	}

	// update overlay ubo
	{
		const float BORDER_WIDTH = 10.0f;
		const float OVERLAY_DIM = 100.0f;

		vertex_pos_normal_s* v = (vertex_pos_normal_s*)MapBuffer(overlay_vertex_buffer_);
		if (v) {

			// overlay rectangle

			/*

			3--2
			| /|
			|/ |
			0--1

			*/

			for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
				float x_left = BORDER_WIDTH + (OVERLAY_DIM + BORDER_WIDTH) * i;
				float x_right = x_left + OVERLAY_DIM;
				float y_top = cfg_viewport_cy_ - BORDER_WIDTH;
				float y_bottom = y_top - OVERLAY_DIM;

				v[0].pos_ = glm::vec3(
					x_left,
					y_bottom,
					0.0f);
				v[0].normal_.x = 0.0f;		// store u
				v[0].normal_.y = 0.0f;		// store v
				v[0].normal_.z = (float)i;	// store shadow map level

				v[1].pos_ = glm::vec3(
					x_right,
					y_bottom,
					0.0f);
				v[1].normal_.x = 1.0f;		// store u
				v[1].normal_.y = 0.0f;		// store v
				v[1].normal_.z = (float)i;	// store shadow map level

				v[2].pos_ = glm::vec3(
					x_right,
					y_top,
					0.0f);
				v[2].normal_.x = 1.0f;		// store u
				v[2].normal_.y = 1.0f;		// store v
				v[2].normal_.z = (float)i;	// store shadow map level

				v[3].pos_ = glm::vec3(
					x_left,
					y_top,
					0.0f);
				v[3].normal_.x = 0.0f;		// store u
				v[3].normal_.y = 1.0f;		// store v
				v[3].normal_.z = (float)i;	// store shadow map level

				v[4] = v[0];
				v[5] = v[2];

				v += 6;
			}

			UnmapBuffer(overlay_vertex_buffer_);
		}
	}

	{
		ubo_mat_s mat;
		mat.matrix_ = glm::orthoRH_ZO(0.0f, (float)cfg_viewport_cx_, 0.0f, (float)cfg_viewport_cy_,
			-1.0f, 1.0f);
		UpdateBuffer(uniform_buffer_overlay_mvp_, &mat, sizeof(mat));
	}

	BuildCommandBuffers();
}

// textures
bool CascadedShadowMapsDemo::LoadTextures() {
	const char* TEX_NAME = "gray.bmp";	// this texture will update later
	if (!Load2DTexture(TEX_NAME, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, vk_sampler_scene_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_terrain_)) {
		return false;
	}

	if (!Load2DTexture("terrain/detailMap.tga", VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, vk_sampler_scene_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_detail_)) {
		return false;
	}

	return true;
}

void CascadedShadowMapsDemo::FreeTextures() {
	DestroyImage(texture_detail_);
	DestroyImage(texture_terrain_);
}

bool CascadedShadowMapsDemo::CreateRenderPass_Depth() {
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

bool CascadedShadowMapsDemo::CreateDepthImage() {
	return Create2DImage(vk_image_depth_, 0, vk_shadow_map_depth_format_, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		DEPTH_FRAMEBUFFER_DIM, DEPTH_FRAMEBUFFER_DIM, SHADOW_MAP_COUNT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, vk_sampler_depth_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

bool CascadedShadowMapsDemo::CreateDepthFramebuffers() {
	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
		// create image view
		VkImageViewCreateInfo image_view_create_info = {};

		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = nullptr;
		image_view_create_info.flags = 0;
		image_view_create_info.image = vk_image_depth_.image_;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		image_view_create_info.format = vk_shadow_map_depth_format_;
		// components
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = i;
		image_view_create_info.subresourceRange.layerCount = 1;

		if (VK_SUCCESS != vkCreateImageView(vk_device_, &image_view_create_info,
			nullptr, &shadow_maps_[i].depth_image_view_)) {
			return false;
		}

		VkFramebufferCreateInfo frame_buffer_create_info = {};

		std::array<VkImageView, 1> attachments = {
			shadow_maps_[i].depth_image_view_
		};

		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.pNext = nullptr;
		frame_buffer_create_info.flags = 0;
		frame_buffer_create_info.renderPass = vk_render_pass_depth_;
		frame_buffer_create_info.attachmentCount = (uint32_t)attachments.size();
		frame_buffer_create_info.pAttachments = attachments.data();
		frame_buffer_create_info.width = DEPTH_FRAMEBUFFER_DIM;
		frame_buffer_create_info.height = DEPTH_FRAMEBUFFER_DIM;
		frame_buffer_create_info.layers = 1;

		if (VK_SUCCESS != vkCreateFramebuffer(vk_device_, &frame_buffer_create_info,
			nullptr, &shadow_maps_[i].framebuffer_))
		{
			return false;
		}
	}

	return true;
}

// uniform buffer
bool CascadedShadowMapsDemo::CreateUniformBuffers() {
	uint32_t ubo_buf_depth_mvp_lst_sz = mvp_buffer_size_ * SHADOW_MAP_COUNT;
	uint32_t ubo_buf_overlay_mvp_lst_sz = mvp_buffer_size_ * SHADOW_MAP_COUNT;

	return CreateBuffer(uniform_buffer_depth_mvp_list_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ubo_buf_depth_mvp_lst_sz) &&
		CreateBuffer(uniform_buffer_overlay_mvp_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			sizeof(ubo_mat_s)) &&
		CreateBuffer(uniform_buffer_mvp_, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_mvp_sep_s)) &&
		CreateBuffer(uniform_buffer_model_matrix_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_mat_s)) &&
		CreateBuffer(uniform_buffer_viewer_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_viewer_s)) &&
		CreateBuffer(uniform_buffer_terrain_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_terrain_s)) &&
		CreateBuffer(uniform_buffer_directional_light_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_directional_light_s)) &&
		CreateBuffer(uniform_buffer_fog_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_fog_s)) &&
		CreateBuffer(uniform_buffer_cas_,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubo_cas_s));
}

void CascadedShadowMapsDemo::DestroyUniformBuffers() {
	DestroyBuffer(uniform_buffer_cas_);
	DestroyBuffer(uniform_buffer_fog_);
	DestroyBuffer(uniform_buffer_directional_light_);
	DestroyBuffer(uniform_buffer_terrain_);
	DestroyBuffer(uniform_buffer_viewer_);
	DestroyBuffer(uniform_buffer_model_matrix_);
	DestroyBuffer(uniform_buffer_mvp_);
	DestroyBuffer(uniform_buffer_overlay_mvp_);
	DestroyBuffer(uniform_buffer_depth_mvp_list_);
}

// vertex buffer

bool CascadedShadowMapsDemo::CreateTerrainBuffers() {

	// create vertex buffer
	size_t vertices_size = sizeof(vertex_pos_normal_s) * SQUARE(vertex_count_per_edge_);
	if (!CreateVertexBuffer(terrain_.vertex_buffer_,
		nullptr, vertices_size, false)) 
	{
		return false;
	}

	// create index buffer
	uint32_t triangle_strip_count = (vertex_count_per_edge_ - 1);
	uint32_t index_per_triangle_strip = vertex_count_per_edge_ * 2;
	uint32_t index_restart_count = triangle_strip_count - 1;
	uint32_t index_count = triangle_strip_count * index_per_triangle_strip + index_restart_count;
	size_t indices_size = sizeof(uint32_t) * index_count;

	if (!CreateIndexBuffer(terrain_.index_buffer_,
		nullptr, indices_size, false)) 
	{
		return false;
	}

	terrain_.index_count_ = index_count;

	return true;
}

void CascadedShadowMapsDemo::DestroyTerrainBuffers() {
	terrain_.index_count_ = 0;
	DestroyBuffer(terrain_.index_buffer_);
	DestroyBuffer(terrain_.vertex_buffer_);
}

bool CascadedShadowMapsDemo::LoadModel() {
	VkModel::load_params_s  load_params = {
		.desc_set_layout_bind0_mat_bind1_tex_ = vk_desc_set_layout_material_texture_
	};
	
	if (tree_.Load(load_params, "tree/tree.obj", false)) {

		const float* min_coords = tree_.GetMin();
		const float* max_coords = tree_.GetMax();

		float model_tree_height = max_coords[1] - min_coords[1];	// model along y-axis

		tree_scale_ = TREE_HEIGHT / model_tree_height;
		tree_z_delta_ = -min_coords[1] * tree_scale_;

		return true;
	}
	else {
		return false;
	}
}

void CascadedShadowMapsDemo::FreeModel() {
	tree_.Free();
}

bool CascadedShadowMapsDemo::CreateInstanceBuffer() {
	return CreateVertexBuffer(instance_buffer_, nullptr, sizeof(instance_pos_vec3_s) * INSTANCE_COUNT, false);
}

void CascadedShadowMapsDemo::DestroyInstanceBuffer() {
	DestroyBuffer(instance_buffer_);
}

bool CascadedShadowMapsDemo::CreateOverlayVertexBuffer() {
	uint32_t vertex_count = 6 * INSTANCE_COUNT;

	// vertex_pos_normal_s: pos, u,v, cascade idx
	return CreateVertexBuffer(overlay_vertex_buffer_, nullptr, sizeof(vertex_pos_normal_s) * vertex_count, false);
}

void CascadedShadowMapsDemo::DestroyOverlayVertexBuffer() {
	DestroyBuffer(overlay_vertex_buffer_);
}

// descriptor set layout
bool CascadedShadowMapsDemo::CreateDescSetLayout_Depth() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 1, VK_SHADER_STAGE_VERTEX_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_depth_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_Overlay() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_overlay_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_MVPViewerDepthTex() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 2, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_mvp_viewer_depth_tex_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_MaterialTexture() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_material_texture_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_Bind0_UBO() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_desc_set_layout_bind0_ubo_);

}

bool CascadedShadowMapsDemo::CreateDescSetLayout_TerrainTex() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// base texture
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 0, VK_SHADER_STAGE_FRAGMENT_BIT);

	// detail texture
	Vk_PushDescriptorSetLayoutBinding_Tex(bindings, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_,
		&create_info, nullptr, &vk_desc_set_layout_terrain_tex_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_Terrain() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// ubo_terrain
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_VERTEX_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, 
		&create_info, nullptr, &vk_desc_set_layout_terrain_);
}

bool CascadedShadowMapsDemo::CreateDescSetLayout_Cas() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// ubo_terrain
	Vk_PushDescriptorSetLayoutBinding_UBO(bindings, 0, VK_SHADER_STAGE_FRAGMENT_BIT);

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = (uint32_t)bindings.size();
	create_info.pBindings = bindings.data();

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_,
		&create_info, nullptr, &vk_desc_set_layout_cas_);
}

// descriptor set
bool CascadedShadowMapsDemo::AllocDescriptorSets() {
	{
		VkDescriptorSetAllocateInfo allocate_info = {};

		std::array<VkDescriptorSetLayout, 8> set_layouts = {
			vk_desc_set_layout_terrain_,
			vk_desc_set_layout_terrain_tex_,
			vk_desc_set_layout_mvp_viewer_depth_tex_,
			vk_desc_set_layout_bind0_ubo_,	// light
			vk_desc_set_layout_bind0_ubo_,	// model matrix
			vk_desc_set_layout_bind0_ubo_,	// fog
			vk_desc_set_layout_overlay_,
			vk_desc_set_layout_cas_
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

		vk_desc_set_terrain_ = sets[0];
		vk_desc_set_terrain_tex_ = sets[1];
		vk_desc_set_mvp_viewer_depth_tex_ = sets[2];
		vk_desc_set_light_ = sets[3];
		vk_desc_set_model_mat_ = sets[4];
		vk_desc_set_fog_ = sets[5];
		vk_desc_set_overlay_ = sets[6];
		vk_desc_set_cas_ = sets[7];

		update_desc_sets_buffer_s buffer;

		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_terrain_, 0, uniform_buffer_terrain_.buffer_, 0, uniform_buffer_terrain_.memory_size_);

		Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_terrain_tex_, 0, texture_terrain_);
		Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_terrain_tex_, 1, texture_detail_);

		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_depth_tex_, 0, uniform_buffer_mvp_.buffer_, 0, uniform_buffer_mvp_.memory_size_);
		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_mvp_viewer_depth_tex_, 1, uniform_buffer_viewer_.buffer_, 0, uniform_buffer_viewer_.memory_size_);
		Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_mvp_viewer_depth_tex_, 2, vk_image_depth_);

		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_light_, 0, uniform_buffer_directional_light_.buffer_, 0, uniform_buffer_directional_light_.memory_size_);
		
		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_model_mat_, 0, uniform_buffer_model_matrix_.buffer_, 0, uniform_buffer_model_matrix_.memory_size_);
		
		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_fog_, 0, uniform_buffer_fog_.buffer_, 0, uniform_buffer_fog_.memory_size_);

		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_overlay_, 0, uniform_buffer_overlay_mvp_.buffer_, 0, uniform_buffer_overlay_mvp_.memory_size_);
		Vk_PushWriteDescriptorSet_Tex(buffer, vk_desc_set_overlay_, 1, vk_image_depth_);

		Vk_PushWriteDescriptorSet_UBO(buffer, vk_desc_set_cas_, 0, uniform_buffer_cas_.buffer_, 0, uniform_buffer_cas_.memory_size_);

		vkUpdateDescriptorSets(vk_device_,
			(uint32_t)buffer.write_descriptor_sets_.size(), buffer.write_descriptor_sets_.data(), 0, nullptr);
	}

	// depth
	{
		VkDescriptorSetAllocateInfo allocate_info = {};

		std::vector<VkDescriptorSetLayout> set_layouts;

		for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
			set_layouts.push_back(vk_desc_set_layout_depth_);
		}

		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.pNext = nullptr;
		allocate_info.descriptorPool = vk_descriptor_pool_;
		allocate_info.descriptorSetCount = (uint32_t)set_layouts.size();
		allocate_info.pSetLayouts = set_layouts.data();

		std::vector<VkDescriptorSet> sets(SHADOW_MAP_COUNT);

		if (VK_SUCCESS != vkAllocateDescriptorSets(vk_device_, &allocate_info, sets.data())) {
			return false;
		}

		update_desc_sets_buffer_s buffer;

		for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
			shadow_maps_[i].desc_set_ = sets[i];
			Vk_PushWriteDescriptorSet_UBO(buffer, sets[i], 0,
				uniform_buffer_depth_mvp_list_.buffer_,
				mvp_buffer_size_ * i,
				mvp_buffer_size_);
			Vk_PushWriteDescriptorSet_UBO(buffer, sets[i], 1, 
				uniform_buffer_model_matrix_.buffer_, 0, uniform_buffer_model_matrix_.memory_size_);
		}

		vkUpdateDescriptorSets(vk_device_,
			(uint32_t)buffer.write_descriptor_sets_.size(), 
			buffer.write_descriptor_sets_.data(), 0, nullptr);
	}

	return true;
}

void CascadedShadowMapsDemo::FreeDescriptorSets() {
	if (shadow_maps_[0].desc_set_) {
		std::vector<VkDescriptorSet> sets;

		for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
			sets.push_back(shadow_maps_[i].desc_set_);
			shadow_maps_[i].desc_set_ = VK_NULL_HANDLE;
		}

		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, (uint32_t)sets.size(), sets.data());
	}

	if (vk_desc_set_terrain_) {
		std::array<VkDescriptorSet, 8> sets = {
			vk_desc_set_terrain_,
			vk_desc_set_terrain_tex_,
			vk_desc_set_mvp_viewer_depth_tex_,
			vk_desc_set_light_,
			vk_desc_set_model_mat_,
			vk_desc_set_fog_,
			vk_desc_set_overlay_,
			vk_desc_set_cas_
		};
		vkFreeDescriptorSets(vk_device_, vk_descriptor_pool_, (uint32_t)sets.size(), sets.data());

		vk_desc_set_terrain_ = VK_NULL_HANDLE;
		vk_desc_set_terrain_tex_ = VK_NULL_HANDLE;
		vk_desc_set_mvp_viewer_depth_tex_ = VK_NULL_HANDLE;
		vk_desc_set_light_ = VK_NULL_HANDLE;
		vk_desc_set_model_mat_ = VK_NULL_HANDLE;
		vk_desc_set_fog_ = VK_NULL_HANDLE;
		vk_desc_set_overlay_ = VK_NULL_HANDLE;
		vk_desc_set_cas_ = VK_NULL_HANDLE;
	}
}

// pipeline
bool CascadedShadowMapsDemo::CreatePipelineLayout_Depth() {
	VkPipelineLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayout, 2> descriptorset_layouts = {
		vk_desc_set_layout_depth_,	// set = 0
		vk_desc_set_layout_material_texture_,	// order 1: set = 1
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
	create_info.pSetLayouts = descriptorset_layouts.data();
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info,
		nullptr, &vk_pipeline_layout_depth_);
}

bool CascadedShadowMapsDemo::CreatePipelineLayout_Overlay() {
	VkPipelineLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayout, 1> descriptorset_layouts = {
		vk_desc_set_layout_overlay_,	// set = 0
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
	create_info.pSetLayouts = descriptorset_layouts.data();
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info,
		nullptr, &vk_pipeline_layout_overlay_);
}

bool CascadedShadowMapsDemo::CreatePipelineLayout_Terrain() {
	VkPipelineLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayout, 6> descriptorset_layouts = {
		vk_desc_set_layout_mvp_viewer_depth_tex_,	// set = 0
		vk_desc_set_layout_terrain_tex_,	// set = 1
		vk_desc_set_layout_bind0_ubo_,		// set = 2
		vk_desc_set_layout_terrain_,		// set = 3
		vk_desc_set_layout_bind0_ubo_,		// set = 4 fog
		vk_desc_set_layout_cas_				// set = 5
	};

	VkPushConstantRange pcr = {
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(const_s)
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
	create_info.pSetLayouts = descriptorset_layouts.data();
	create_info.pushConstantRangeCount = 1;
	create_info.pPushConstantRanges = &pcr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info, 
		nullptr, &vk_pipeline_layout_terrain_);

}

bool CascadedShadowMapsDemo::CreatePipelineLayout_Vegetation() {
	VkPipelineLayoutCreateInfo create_info = {};

	std::array<VkDescriptorSetLayout, 6> descriptorset_layouts = {
		vk_desc_set_layout_mvp_viewer_depth_tex_,	// order 0: set = 0
		vk_desc_set_layout_material_texture_,	// order 1: set = 1
		vk_desc_set_layout_bind0_ubo_,			// order 2: set = 2, light
		vk_desc_set_layout_bind0_ubo_,			// order 3: set = 3, model matrix
		vk_desc_set_layout_bind0_ubo_,			// order 4: set = 4, fog
		vk_desc_set_layout_cas_					// order 5: set = 5
	};

	VkPushConstantRange pcr = {
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(const_s)
	};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = (uint32_t)descriptorset_layouts.size();
	create_info.pSetLayouts = descriptorset_layouts.data();
	create_info.pushConstantRangeCount = 1;
	create_info.pPushConstantRanges = &pcr;

	return VK_SUCCESS == vkCreatePipelineLayout(vk_device_, &create_info,
		nullptr, &vk_pipeline_layout_vegetation_);
}

bool CascadedShadowMapsDemo::CreatePipeline_Overlay() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/overlay.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/overlay.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_NORMAL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_FALSE,
		.depth_bias_constant_factor_ = 0.0f,
		.depth_bias_slope_factor_ = 0.0f,
		.depth_test_enable_ = VK_FALSE,
		.depth_write_enable_ = VK_FALSE,
		.pipeline_layout_ = vk_pipeline_layout_overlay_,
		.render_pass_ = vk_render_pass_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_overlay_);
}

bool CascadedShadowMapsDemo::CreatePipeline_Depth_Terrain() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/depth_terrain.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/depth_terrain.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = 0,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		.primitive_restart_enable_ = VK_TRUE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_NONE,	// disable backface culling
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_FALSE,
		.depth_bias_constant_factor_ = 0.0f,
		.depth_bias_slope_factor_ = 0.0f,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_depth_,
		.render_pass_ = vk_render_pass_depth_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_depth_terrain_);
}

bool CascadedShadowMapsDemo::CreatePipeline_Depth_Vegetation() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/depth_inst.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/depth_inst.frag.spv",
		.vertex_format_ = tree_.GetVertexFormat(),
		.instance_format_ = instance_format_t::INST_POS_VEC3,
		.additional_vertex_fields_ = VERTEX_FIELD_UV,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_NONE,	// disable backface culling
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_FALSE,
		.depth_bias_constant_factor_ = 0.0f,
		.depth_bias_slope_factor_ = 0.0f,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_depth_,
		.render_pass_ = vk_render_pass_depth_
	};

	return CreatePipelineVertFrag(params, vk_pipeline_depth_vegetation_);
}

bool CascadedShadowMapsDemo::CreatePipelines_Terrain() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/terrain.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/terrain.frag.spv",
		.vertex_format_ = vertex_format_t::VF_POS_NORMAL,
		.instance_format_ = instance_format_t::INST_NONE,
		.additional_vertex_fields_ = VERTEX_FIELD_ALL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		.primitive_restart_enable_ = VK_TRUE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_TRUE,
		.depth_bias_constant_factor_ = 4096.0f,
		.depth_bias_slope_factor_ = 1.0f,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_terrain_,
		.render_pass_ = vk_render_pass_
	};

	if (!CreatePipelineVertFrag(params, vk_pipeline_terrain_fill_)) {
		return false;
	}

	params.polygon_mode_ = VK_POLYGON_MODE_LINE;

	return CreatePipelineVertFrag(params, vk_pipeline_terrain_line_);
}

bool CascadedShadowMapsDemo::CreatePipelines_Vegetation_Mat() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/vegetation.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/vegetation.frag.spv",
		.vertex_format_ = tree_.GetVertexFormat(),
		.instance_format_ = instance_format_t::INST_POS_VEC3,
		.additional_vertex_fields_ = VERTEX_FIELD_NORMAL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_FALSE,
		.depth_bias_constant_factor_ = 0.0f,
		.depth_bias_slope_factor_ = 0.0f,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_vegetation_,
		.render_pass_ = vk_render_pass_
	};

	if (!CreatePipelineVertFrag(params, vk_pipeline_vegetation_mat_fill_)) {
		return false;
	}

	params.polygon_mode_ = VK_POLYGON_MODE_LINE;

	return CreatePipelineVertFrag(params, vk_pipeline_vegetation_mat_line_);
}

bool CascadedShadowMapsDemo::CreatePipelines_Vegetation_Tex() {
	create_pipeline_vert_frag_params_s params = {
		.vertex_shader_filename_ = "SPIR-V/vegetation_tex.vert.spv",
		.framgment_shader_filename_ = "SPIR-V/vegetation_tex.frag.spv",
		.vertex_format_ = tree_.GetVertexFormat(),
		.instance_format_ = instance_format_t::INST_POS_VEC3,
		.additional_vertex_fields_ = VERTEX_FIELD_ALL,
		.primitive_topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitive_restart_enable_ = VK_FALSE,
		.polygon_mode_ = VK_POLYGON_MODE_FILL,
		.cull_mode_ = VK_CULL_MODE_BACK_BIT,
		.front_face_ = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depth_bias_enable_ = VK_FALSE,
		.depth_bias_constant_factor_ = 0.0f,
		.depth_bias_slope_factor_ = 0.0f,
		.depth_test_enable_ = VK_TRUE,
		.depth_write_enable_ = VK_TRUE,
		.pipeline_layout_ = vk_pipeline_layout_vegetation_,
		.render_pass_ = vk_render_pass_
	};

	if (!CreatePipelineVertFrag(params, vk_pipeline_vegetation_tex_fill_)) {
		return false;
	}

	params.polygon_mode_ = VK_POLYGON_MODE_LINE;

	return CreatePipelineVertFrag(params, vk_pipeline_vegetation_tex_line_);
}

void CascadedShadowMapsDemo::UpdateTerrain() {
	SRand(random_seed_++);

	terrain_gen_params_s terrain_gen_params = {
		.algo_ = terrain_gen_algorithm_t::FAULT_FORMATION,
		.sz_ = TERRAIN_SIZE,
		.min_z_ = 0.0f,
		.max_z_ = TERRAIN_START_DELTA_Z,
		.iterations_ = 128,
		.filter_ = 0.4f,	// 0.15 ~ 0.75
		.roughness_ = 0.0f	// not used
	};

	terrain_s terrain = {};

	if (!Terrain_Generate(terrain_gen_params, terrain)) {
		printf("Could not generate a test terrain\n");
		return;
	}

	uint32_t vertex_count = SQUARE(vertex_count_per_edge_);

	// fill instance buffer
	int n = (int)terrain_edge_length_ - 1;

	int* occupied = (int*)TEMP_ALLOC(sizeof(int) * vertex_count);
	if (!occupied) {
		Terrain_Free(terrain);
		return;
	}
	
	memset(occupied, 0, sizeof(int) * vertex_count);

	for (int i = 0; i < INSTANCE_COUNT; ++i) {

		while (true) {
			uint32_t x = Rand() % n + 1;
			uint32_t y = Rand() % n + 1;

			if (!occupied[y * vertex_count_per_edge_ + x]) {
				instance_buf_[i].pos_.x = (float)x;
				instance_buf_[i].pos_.y = (float)y;
				instance_buf_[i].pos_.z = terrain.heights_[y * vertex_count_per_edge_ + x] + tree_z_delta_;

				// scale
				instance_buf_[i].vec3_ = glm::vec3(tree_scale_);

				occupied[y * vertex_count_per_edge_ + x] = 1;
				break;
			}
		}

	}

	float max_z = 0.0f;
	for (uint32_t i = 0; i < vertex_count; ++i) {
		float z = terrain.heights_[i];
		if (z > max_z) {
			max_z = z;
		}
	}

	/*

	 z   y
	 |  /
	 | /
	 |/___x

	 */

	 // z axis is up

	// create vertex buffer
	size_t vertices_size = sizeof(vertex_pos_normal_s) * vertex_count;
	vertex_pos_normal_s * vertices = (vertex_pos_normal_s*)TEMP_ALLOC(vertices_size);
	if (!vertices) {
		Terrain_Free(terrain);
		return;
	}

	struct normal_calc_s {
		glm::vec3		normal_sum_;
		uint32_t		normal_count_;
	};

	size_t normal_calc_vertices_size = sizeof(normal_calc_s) * vertex_count;
	normal_calc_s * normal_calc_vertices = (normal_calc_s*)TEMP_ALLOC(normal_calc_vertices_size);
	if (!normal_calc_vertices) {
		TEMP_FREE(vertices);
		Terrain_Free(terrain);
		return;
	}
	memset(normal_calc_vertices, 0, normal_calc_vertices_size);

	const float* z = terrain.heights_;
	const float* r = z;
	vertex_pos_normal_s * v = vertices;
	for (uint32_t y = 0; y < vertex_count_per_edge_; ++y) {
		for (uint32_t x = 0; x < vertex_count_per_edge_; ++x) {
			/*

			y
			|
			|   3--2
			|   | /|
			|	|/ |
			|	0--1
			|
			 ---------------- x

			*/

			v->pos_.x = (float)x;
			v->pos_.y = (float)y;
			v->pos_.z = r[x];
			v->normal_.x = 0.0f;
			v->normal_.y = 0.0f;
			v->normal_.z = 1.0f;

			v++;
		}
		r += vertex_count_per_edge_;	// next row
	}

	// calculate normal
	for (uint32_t y = 0; y < vertex_count_per_edge_ - 1; ++y) {
		for (uint32_t x = 0; x < vertex_count_per_edge_ - 1; ++x) {
			/*

			y
			|
			|   3--2
			|   | /|
			|	|/ |
			|	0--1
			|
			 ---------------- x

			*/

			uint32_t v0_idx = y * vertex_count_per_edge_ + x;
			uint32_t v1_idx = v0_idx + 1;
			uint32_t v2_idx = v1_idx + vertex_count_per_edge_;
			uint32_t v3_idx = v0_idx + vertex_count_per_edge_;

			glm::vec3 v1_v0 = vertices[v1_idx].pos_ - vertices[v0_idx].pos_;
			glm::vec3 v2_v0 = vertices[v2_idx].pos_ - vertices[v0_idx].pos_;

			glm::vec3 normal1 = glm::normalize(glm::cross(v1_v0, v2_v0));

			normal_calc_vertices[v0_idx].normal_sum_ += normal1;
			normal_calc_vertices[v1_idx].normal_sum_ += normal1;
			normal_calc_vertices[v2_idx].normal_sum_ += normal1;

			normal_calc_vertices[v0_idx].normal_count_++;
			normal_calc_vertices[v1_idx].normal_count_++;
			normal_calc_vertices[v2_idx].normal_count_++;

			glm::vec3 v3_v0 = vertices[v3_idx].pos_ - vertices[v0_idx].pos_;

			glm::vec3 normal2 = glm::normalize(glm::cross(v2_v0, v3_v0));

			normal_calc_vertices[v0_idx].normal_sum_ += normal2;
			normal_calc_vertices[v2_idx].normal_sum_ += normal2;
			normal_calc_vertices[v3_idx].normal_sum_ += normal2;

			normal_calc_vertices[v0_idx].normal_count_++;
			normal_calc_vertices[v2_idx].normal_count_++;
			normal_calc_vertices[v3_idx].normal_count_++;
		}
	}

	for (uint32_t i = 0; i < vertex_count; ++i) {
		if (normal_calc_vertices[i].normal_count_) {
			float f = 1.0f / normal_calc_vertices[i].normal_count_;
			vertices[i].normal_ = glm::normalize(normal_calc_vertices[i].normal_sum_ * f);
		}
	}

	TEMP_FREE(normal_calc_vertices);
	normal_calc_vertices = nullptr;

	// update base texture
	image_s new_texture = {};
	if (Terrain_Texture(texture_tiles_, terrain,
		texture_terrain_.width_, texture_terrain_.height_, new_texture)) {
		Update2DTexture(new_texture, texture_terrain_);
		Img_Free(new_texture);
	}

	Terrain_Free(terrain);

	if (!UpdateBuffer(terrain_.vertex_buffer_, vertices, vertices_size)) {
		TEMP_FREE(vertices);
		printf("UpdateBuffer failed\n");
		return;
	}

	TEMP_FREE(vertices);

	// setup index buffer
	uint32_t triangle_strip_count = (vertex_count_per_edge_ - 1);
	uint32_t index_per_triangle_strip = vertex_count_per_edge_ * 2;
	uint32_t index_restart_count = triangle_strip_count - 1;
	uint32_t index_count = triangle_strip_count * index_per_triangle_strip + index_restart_count;
	size_t indices_size = sizeof(uint32_t) * index_count;

	uint32_t* indices = (uint32_t*)TEMP_ALLOC(indices_size);
	if (!indices) {
		return;
	}

	uint32_t* idx = indices;
	for (uint32_t y = 0; y < triangle_strip_count; ++y) {
		if (y > 0) {
			*idx++ = VK_INVALID_INDEX;	// triangle strip restart
		}

		for (uint32_t x = 0; x < vertex_count_per_edge_; ++x) {
			uint32_t i = y * vertex_count_per_edge_ + x;

			*idx++ = i + vertex_count_per_edge_;
			*idx++ = i;
		}
	}

	if (!UpdateBuffer(terrain_.index_buffer_, indices, indices_size)) {
		TEMP_FREE(indices);
		printf("UpdateBuffer failed\n");
		return;
	}

	TEMP_FREE(indices);

	// update instance buffer

	UpdateBuffer(instance_buffer_, instance_buf_, sizeof(instance_buf_));
}

// build command buffer
void CascadedShadowMapsDemo::BuildCommandBuffer(const scene_pipelines_s& scene_pipelines) {
	uint32_t sz_draw_cmd_buffer = (uint32_t)vk_draw_cmd_buffer_count_;
	for (uint32_t i = 0; i < sz_draw_cmd_buffer; ++i) {
		BuildOneCommandBuffer(vk_draw_cmd_buffers_[i], vk_framebuffers_[i], scene_pipelines);
	}
}

void CascadedShadowMapsDemo::BuildOneCommandBuffer(
	VkCommandBuffer cmd_buf, VkFramebuffer scene_fb, const scene_pipelines_s& scene_pipelines)
{
	VkCommandBufferBeginInfo cmd_buf_begin_info = {};

	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = 0;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);

	BuildCommandBuffer_DepthPasses(cmd_buf);
	BuildCommandBuffer_ScenePass(cmd_buf, scene_fb, scene_pipelines);

	VkResult rt = vkEndCommandBuffer(cmd_buf);
	if (rt != VK_SUCCESS) {
		printf("vkEndCommandBuffer error\n");
	}
}

void CascadedShadowMapsDemo::BuildCommandBuffer_DepthPasses(VkCommandBuffer cmd_buf) {
	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; ++i) {
		BuildCommandBuffer_DepthPass(cmd_buf, shadow_maps_[i]);
	}
}

void CascadedShadowMapsDemo::BuildCommandBuffer_ScenePass(
	VkCommandBuffer cmd_buf, VkFramebuffer scene_fb, const scene_pipelines_s& scene_pipelines)
{
	VkRenderPassBeginInfo render_pass_begin_info = {};

	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;

	render_pass_begin_info.renderPass = vk_render_pass_;
	render_pass_begin_info.framebuffer = scene_fb;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = cfg_viewport_cx_;
	render_pass_begin_info.renderArea.extent.height = cfg_viewport_cy_;

	VkClearValue clear_values[2];

	clear_values[0].color.float32[0] = sky_color_.r;
	clear_values[0].color.float32[1] = sky_color_.g;
	clear_values[0].color.float32[2] = sky_color_.b;
	clear_values[0].color.float32[3] = 1.0f;
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	render_pass_begin_info.clearValueCount = 2; // color & depth-stencil
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

	// draw terrain
	{
		std::array<VkDescriptorSet, 6> descriptor_sets = {
			vk_desc_set_mvp_viewer_depth_tex_,
			vk_desc_set_terrain_tex_,
			vk_desc_set_light_,
			vk_desc_set_terrain_,
			vk_desc_set_fog_,
			vk_desc_set_cas_
		};

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_terrain_, 0, (uint32_t)descriptor_sets.size(),
			descriptor_sets.data(), 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipelines.pipeline_terrain_);

		const_s c;
		c.options_.x = draw_debug_ ? 1u : 0u;
		c.options_.y = draw_fog_ ? 1u : 0u;

		vkCmdPushConstants(cmd_buf, vk_pipeline_layout_terrain_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(c), &c);

		VkDeviceSize offset[1] = { 0 };
		vkCmdBindVertexBuffers(cmd_buf,
			0, // index of the first vertex input binding
			1, &terrain_.vertex_buffer_.buffer_, offset);

		vkCmdBindIndexBuffer(cmd_buf,
			terrain_.index_buffer_.buffer_,
			0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd_buf, terrain_.index_count_, 1, 0, 0, 0);
	}

	// draw vegetation (instance mode)
	if (draw_vegetation_) {

		VkDeviceSize offset[1] = { 0 };
		VkBuffer vertex_buffer = tree_.GetVertexBuffer();
		vkCmdBindVertexBuffers(cmd_buf,
			0,
			1,
			&vertex_buffer, offset);

		VkBuffer index_buffer = tree_.GetIndexBuffer();
		vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindVertexBuffers(cmd_buf,
			1, // index of the first vertex input binding
			1,
			&instance_buffer_.buffer_, offset);

		uint32_t model_part_count = tree_.GetModelPartCount();

		uint32_t material_count = tree_.GetMaterialCount();
		for (uint32_t j = 0; j < material_count; ++j) {
			const VkModel::vk_material_s* vk_mat = tree_.GetMaterialByIdx(j);

			std::array<VkDescriptorSet, 6> descriptor_sets = {
				vk_desc_set_mvp_viewer_depth_tex_,
				vk_mat->vk_desc_set_,
				vk_desc_set_light_,
				vk_desc_set_model_mat_,
				vk_desc_set_fog_,
				vk_desc_set_cas_
			};

			vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				vk_pipeline_layout_vegetation_, 0,
				(uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

			if (vk_mat->vk_texture_.image_) {
				// texture
				vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipelines.pipeline_vegetation_tex_);
			}
			else {
				// non-texture
				vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipelines.pipeline_vegetation_mat_);
			}

			const_s c;
			c.options_.x = draw_debug_ ? 1u : 0u;
			c.options_.y = draw_fog_ ? 1u : 0u;
			vkCmdPushConstants(cmd_buf, vk_pipeline_layout_vegetation_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(c), &c);

			for (uint32_t k = 0; k < model_part_count; ++k) {
				const VkModel::vk_model_part_s* model_part = tree_.GetModelPartByIdx(k);
				if (model_part->material_idx_ == j) {
					vkCmdDrawIndexed(cmd_buf, model_part->index_count_,
						INSTANCE_COUNT,
						model_part->index_offset_,
						0, 0);
				}
			}
		}
	}

	if (draw_overlay_) {
		VkDescriptorSet descriptor_sets[1] = {};
		VkDeviceSize offset[1] = { 0 };

		// draw terrain
		descriptor_sets[0] = vk_desc_set_overlay_;

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_pipeline_layout_overlay_, 0, 1, descriptor_sets, 0, nullptr);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_overlay_);

		offset[0] = 0;
		vkCmdBindVertexBuffers(cmd_buf,
			0,
			1,
			&overlay_vertex_buffer_.buffer_, offset);

		vkCmdDraw(cmd_buf, 6 * SHADOW_MAP_COUNT, 1, 0, 0);
	}

	vkCmdEndRenderPass(cmd_buf);
}

void CascadedShadowMapsDemo::BuildCommandBuffer_DepthPass(VkCommandBuffer cmd_buf, const shadow_map_s& shadow_map) {
	VkRenderPassBeginInfo render_pass_begin_info = {};

	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;

	render_pass_begin_info.renderPass = vk_render_pass_depth_;
	render_pass_begin_info.framebuffer = shadow_map.framebuffer_;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = DEPTH_FRAMEBUFFER_DIM;
	render_pass_begin_info.renderArea.extent.height = DEPTH_FRAMEBUFFER_DIM;

	VkClearValue clear_values[1];

	clear_values[0].depthStencil.depth = 1.0f;
	clear_values[0].depthStencil.stencil = 0;

	render_pass_begin_info.clearValueCount = 1; // depth-stencil
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = DEPTH_FRAMEBUFFER_DIM,
		.height = DEPTH_FRAMEBUFFER_DIM,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = DEPTH_FRAMEBUFFER_DIM;
	scissor.extent.height = DEPTH_FRAMEBUFFER_DIM;

	vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

	VkDescriptorSet descriptor_sets[1] = {};
	VkDeviceSize offset[1] = { 0 };

	// draw terrain
	descriptor_sets[0] = shadow_map.desc_set_;

	vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_pipeline_layout_depth_, 0, 1, descriptor_sets, 0, nullptr);

	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_depth_terrain_);

	vkCmdBindVertexBuffers(cmd_buf,
		0, // index of the first vertex input binding
		1, &terrain_.vertex_buffer_.buffer_, offset);

	vkCmdBindIndexBuffer(cmd_buf,
		terrain_.index_buffer_.buffer_,
		0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd_buf, terrain_.index_count_, 1, 0, 0, 0);

	// draw vegetation (instance mode)
	if (draw_vegetation_) {

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_depth_vegetation_);

		offset[0] = 0;
		VkBuffer vertex_buffer = tree_.GetVertexBuffer();
		vkCmdBindVertexBuffers(cmd_buf,
			0,
			1,
			&vertex_buffer, offset);

		VkBuffer index_buffer = tree_.GetIndexBuffer();
		vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindVertexBuffers(cmd_buf,
			1, // index of the first vertex input binding
			1,
			&instance_buffer_.buffer_, offset);


		uint32_t model_part_count = tree_.GetModelPartCount();

		uint32_t material_count = tree_.GetMaterialCount();
		for (uint32_t j = 0; j < material_count; ++j) {
			const VkModel::vk_material_s* vk_mat = tree_.GetMaterialByIdx(j);

			std::array<VkDescriptorSet, 2> descriptor_sets = {
				shadow_map.desc_set_,
				vk_mat->vk_desc_set_
			};

			vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				vk_pipeline_layout_depth_, 0,
				(uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

			if (vk_mat->vk_texture_.image_) {
				// texture
				vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_depth_vegetation_);	// TODO

				for (uint32_t k = 0; k < model_part_count; ++k) {
					const VkModel::vk_model_part_s* model_part = tree_.GetModelPartByIdx(k);
					if (model_part->material_idx_ == j) {
						vkCmdDrawIndexed(cmd_buf, model_part->index_count_,
							INSTANCE_COUNT,
							model_part->index_offset_,
							0, 0);
					}
				}
			}
			else {
				// non-texture
				// TODO
			}

		}
	}

	vkCmdEndRenderPass(cmd_buf);
}

void CascadedShadowMapsDemo::UpdateMVPViewerUniformBuffers() {
	ubo_mvp_sep_s ubo_mvp_sep = {};

	GetProjMatrix(ubo_mvp_sep.proj_);
	GetViewMatrix(ubo_mvp_sep.view_);
	GetModelMatrix(ubo_mvp_sep.model_);

	glm::mat4 mvp = ubo_mvp_sep.proj_ * ubo_mvp_sep.view_ * ubo_mvp_sep.model_;

	ubo_cas_.inv_view_mvp_ = glm::inverse(mvp);

	UpdateBuffer(uniform_buffer_mvp_, &ubo_mvp_sep, sizeof(ubo_mvp_sep));

	ubo_viewer_s ubo_viewer = {};

	ubo_viewer.pos_ = glm::vec4(camera_.pos_, 1.0f);

	UpdateBuffer(uniform_buffer_viewer_, &ubo_viewer, sizeof(ubo_viewer));

	for (uint32_t i = 0; i < SHADOW_MAP_COUNT; i++) {
		float f = shadow_maps_[i].z_far_;

		ubo_cas_.z_far_[i].x = (-f * ubo_mvp_sep.proj_[2].z + ubo_mvp_sep.proj_[3].z) / f;
	}
}

void CascadedShadowMapsDemo::SetupTerrainUniformBuffer() {
	ubo_terrain_s ubo_terrain = { terrain_edge_length_ };

	UpdateBuffer(uniform_buffer_terrain_, &ubo_terrain, sizeof(ubo_terrain));
}

void CascadedShadowMapsDemo::SetupModelMatrixUniformBuffer() {
	ubo_mat_s ubo = {};
	ubo.matrix_ = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	UpdateBuffer(uniform_buffer_model_matrix_, &ubo, sizeof(ubo));
}

void CascadedShadowMapsDemo::SetupLightUniformBuffer() {
	ubo_directional_light_s light = {};

	glm::vec3 light_dir;
	CalcLightDir(light_dir);
	light.dir_ = glm::vec4(light_dir, 0.0f);
	light.color_ = sky_color_;

	UpdateBuffer(uniform_buffer_directional_light_, &light, sizeof(light));
}

void CascadedShadowMapsDemo::SetupFogUniformBuffer() {
	ubo_fog_s fog = {};

	fog.params_.x = fog_start_;
	fog.params_.y = z_far_;
	fog.params_.z = 1.0f / (z_far_ - fog_start_);
	fog.params_.w = 0.4f;
	fog.color_ = sky_color_;

	// printf("fog color: %f,%f,%f\n", sky_color_.r, sky_color_.g, sky_color_.b);

	UpdateBuffer(uniform_buffer_fog_, &fog, sizeof(fog));
}

void CascadedShadowMapsDemo::CalcLightDir(glm::vec3& light_dir) const {
	float r = glm::radians(light_angle_);
	light_dir.x = -std::cosf(r);
	light_dir.y = 0.0f;
	light_dir.z = -std::sinf(r);
}

void CascadedShadowMapsDemo::UpdateSkyColor() {
	glm::vec3 light_dir;
	CalcLightDir(light_dir);

	// ref: nvidia exmplae
	sky_color_.r = 0.8f;
	sky_color_.g = -light_dir.z * 0.1f + 0.7f;
	sky_color_.b = -light_dir.z * 0.4f + 0.5f;
	sky_color_.a = 1.0f;
	//printf("UpdateSkyColor: %f\n", light_dir.z);
}

/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(CascadedShadowMapsDemo)
