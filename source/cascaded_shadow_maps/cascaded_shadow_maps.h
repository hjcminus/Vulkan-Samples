/******************************************************************************
 cascaded shadow maps
 *****************************************************************************/

#pragma once

/*
================================================================================
CascadedShadowMapsDemo
================================================================================
*/
class CascadedShadowMapsDemo : public VkDemo {
public:
	CascadedShadowMapsDemo();
	~CascadedShadowMapsDemo() override;

	bool					Init();
	void					Shutdown();
	void					BuildCommandBuffers() override;
	void					WindowSizeChanged() override;
	void					Update() override;

protected:

	void					FuncKeyDown(uint32_t key) override;

private:

	static const uint32_t	SHADOW_MAP_COUNT = 4;
	static const uint32_t	DEPTH_FRAMEBUFFER_DIM = 2048;  // 1024, 2048, 4096

	static constexpr float	TREE_HEIGHT = 8.0f;

	struct alignas(16) ubo_terrain_s {
		float				edge_length_;
	};

	struct alignas(16) ubo_cas_s {
		glm::mat4			inv_view_mvp_;
		glm::vec4			z_far_[SHADOW_MAP_COUNT];	// each element is 16 bytes-aligned
		glm::mat4			light_vp[SHADOW_MAP_COUNT];
	} ubo_cas_;

	struct vk_terrain_s {
		vk_buffer_s			vertex_buffer_;
		vk_buffer_s			index_buffer_;
		uint32_t			index_count_;
	};

	struct shadow_map_s {
		VkDescriptorSet		desc_set_;
		VkImageView			depth_image_view_;
		VkFramebuffer		framebuffer_;
		float				z_near_;
		float				z_far_;
		float				frustum_radius_;	// this cascaded frustum
	} shadow_maps_[SHADOW_MAP_COUNT];

	struct const_s {
		glm::uvec4			options_;	// x: draw_debug, y: draw_fog
	};

	// sampler
	VkSampler				vk_sampler_depth_;
	VkSampler				vk_sampler_scene_;

	vk_image_s				texture_terrain_;
	vk_image_s				texture_detail_;
	vk_image_s				vk_image_depth_;

	VkFormat				vk_shadow_map_depth_format_;
	VkRenderPass			vk_render_pass_depth_;

	uint32_t				mvp_buffer_size_;	// each mvp matrix needed size in VkBuffer

	vk_buffer_s				uniform_buffer_depth_mvp_list_;
	vk_buffer_s				uniform_buffer_overlay_mvp_;

	vk_buffer_s				uniform_buffer_mvp_;
	vk_buffer_s				uniform_buffer_model_matrix_;
	vk_buffer_s				uniform_buffer_viewer_;
	vk_buffer_s				uniform_buffer_terrain_;
	vk_buffer_s				uniform_buffer_directional_light_;
	vk_buffer_s				uniform_buffer_fog_;
	vk_buffer_s				uniform_buffer_cas_;

	// terrain
	float					terrain_edge_length_;
	vk_terrain_s			terrain_;
	VkModel					tree_;
	float					tree_scale_;
	float					tree_z_delta_;
	vk_buffer_s				instance_buffer_;

	// overlay
	vk_buffer_s				overlay_vertex_buffer_;

	// descriptor
	VkDescriptorSetLayout	vk_desc_set_layout_depth_;
	VkDescriptorSetLayout	vk_desc_set_layout_overlay_;
	VkDescriptorSetLayout	vk_desc_set_layout_mvp_viewer_depth_tex_;
	VkDescriptorSetLayout	vk_desc_set_layout_terrain_tex_;
	VkDescriptorSetLayout	vk_desc_set_layout_material_texture_;	// model
	VkDescriptorSetLayout	vk_desc_set_layout_bind0_ubo_;
	VkDescriptorSetLayout   vk_desc_set_layout_terrain_;
	VkDescriptorSetLayout	vk_desc_set_layout_cas_;

	VkDescriptorSet			vk_desc_set_overlay_;
	VkDescriptorSet			vk_desc_set_terrain_tex_;
	VkDescriptorSet         vk_desc_set_terrain_;
	VkDescriptorSet			vk_desc_set_mvp_viewer_depth_tex_;
	VkDescriptorSet			vk_desc_set_light_;
	VkDescriptorSet			vk_desc_set_model_mat_;
	VkDescriptorSet			vk_desc_set_fog_;
	VkDescriptorSet			vk_desc_set_cas_;

	// pipeline
	VkPipelineLayout		vk_pipeline_layout_depth_;
	VkPipelineLayout		vk_pipeline_layout_overlay_;
	VkPipelineLayout		vk_pipeline_layout_terrain_;
	VkPipelineLayout		vk_pipeline_layout_vegetation_;

	VkPipeline				vk_pipeline_depth_terrain_;
	VkPipeline				vk_pipeline_depth_vegetation_;
	VkPipeline				vk_pipeline_overlay_;
	VkPipeline				vk_pipeline_terrain_fill_;
	VkPipeline				vk_pipeline_terrain_line_;
	VkPipeline				vk_pipeline_vegetation_mat_fill_;
	VkPipeline				vk_pipeline_vegetation_mat_line_;
	VkPipeline				vk_pipeline_vegetation_tex_fill_;
	VkPipeline				vk_pipeline_vegetation_tex_line_;

	bool					wireframe_mode_;
	bool					draw_vegetation_;
	bool					draw_overlay_;
	bool					draw_debug_;
	bool					draw_fog_;
	uint32_t				random_seed_;
	static const terrain_size_t	TERRAIN_SIZE = terrain_size_t::TS_128;
	const float				TERRAIN_START_DELTA_Z = 2.5f;
	uint32_t				vertex_count_per_edge_;

	static const uint32_t	INSTANCE_COUNT = 128;	// 128, 256, 512
	instance_pos_vec3_s		instance_buf_[INSTANCE_COUNT];

	struct scene_pipelines_s {
		VkPipeline			pipeline_terrain_;
		VkPipeline			pipeline_vegetation_mat_;
		VkPipeline			pipeline_vegetation_tex_;
	};

	terrain_texture_tiles_s texture_tiles_;

	float					light_angle_;	// from 0 to 180, dir change from <-1, 0  0> to <1, 0, 0>
	float					fog_start_;
	glm::vec4				sky_color_;

	void					OnViewportChanged();

	// textures
	bool					LoadTextures();
	void					FreeTextures();

	bool					CreateRenderPass_Depth();

	bool					CreateDepthImage();

	bool					CreateDepthFramebuffers();

	// uniform buffer
	bool					CreateUniformBuffers();
	void					DestroyUniformBuffers();

	bool					CreateTerrainBuffers();
	void					DestroyTerrainBuffers();

	bool					LoadModel();
	void					FreeModel();

	bool					CreateInstanceBuffer();
	void					DestroyInstanceBuffer();

	bool					CreateOverlayVertexBuffer();
	void					DestroyOverlayVertexBuffer();

	// descriptor set layout
	bool					CreateDescSetLayout_Depth();
	bool					CreateDescSetLayout_Overlay();
	bool					CreateDescSetLayout_MVPViewerDepthTex();
	bool					CreateDescSetLayout_MaterialTexture();
	bool					CreateDescSetLayout_Bind0_UBO();	// light, model matrix
	bool					CreateDescSetLayout_TerrainTex();
	bool					CreateDescSetLayout_Terrain();
	bool					CreateDescSetLayout_Cas();

	// descriptor set
	bool					AllocDescriptorSets();
	void					FreeDescriptorSets();

	// pipeline
	bool					CreatePipelineLayout_Depth();
	bool					CreatePipelineLayout_Overlay();
	bool					CreatePipelineLayout_Terrain();
	bool					CreatePipelineLayout_Vegetation();

	bool					CreatePipeline_Overlay();
	bool					CreatePipeline_Depth_Terrain();
	bool					CreatePipeline_Depth_Vegetation();
	bool					CreatePipelines_Terrain();
	bool					CreatePipelines_Vegetation_Mat();
	bool					CreatePipelines_Vegetation_Tex();

	void					UpdateTerrain();

	// build command buffer
	void					BuildCommandBuffer(const scene_pipelines_s& scene_pipelines);
	
	void					BuildOneCommandBuffer(
								VkCommandBuffer cmd_buf, VkFramebuffer scene_fb, const scene_pipelines_s& scene_pipelines);
	void					BuildCommandBuffer_DepthPasses(VkCommandBuffer cmd_buf);
	void					BuildCommandBuffer_ScenePass(
								VkCommandBuffer cmd_buf, VkFramebuffer scene_fb, const scene_pipelines_s& scene_pipelines);
	void					BuildCommandBuffer_DepthPass(VkCommandBuffer cmd_buf, const shadow_map_s & shadow_map);

	void					UpdateMVPViewerUniformBuffers();
	void					SetupTerrainUniformBuffer();
	void					SetupModelMatrixUniformBuffer();
	void					SetupLightUniformBuffer();
	void					SetupFogUniformBuffer();

	// toward scene
	void					CalcLightDir(glm::vec3 & light_dir) const;
	void					UpdateSkyColor();

};
