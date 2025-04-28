/******************************************************************************
 Demo base class
 *****************************************************************************/

#pragma once

/*
================================================================================
VkDemo
================================================================================
*/
class COMMON_API VkDemo {
public:
	VkDemo();
	virtual ~VkDemo();

	bool					Init(const char* project_shader_dir,
                                uint32_t max_uniform_buffer,
                                uint32_t max_storage_buffer,
                                uint32_t max_texture,
                                uint32_t max_desp_set);
	void					Shutdown();
    virtual void			Display();
    virtual void			BuildCommandBuffers() = 0;
    virtual void            WindowSizeChanged();
    virtual void            Update();

    void                    MainLoop();

    // interface
    VkDevice                GetDevice() const;
    VkDescriptorPool        GetDescriptorPool() const;

    size_t                  GetAlignedBufferSize(size_t sz) const;
    size_t                  GetAlignedMinOffsetSize(size_t sz) const;

    bool                    LoadModel(const char * filename, bool move_to_origin, model_s & model, const glm::mat4* transform = nullptr) const;

    // desc_buffer_info_count can be zero
    bool                    CreateBuffer(vk_buffer_s& buffer, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_prop_flags, VkDeviceSize req_size) const;
    void                    DestroyBuffer(vk_buffer_s& buffer) const;
    bool					UpdateBuffer(vk_buffer_s& buffer, const void* host_data, size_t host_data_size) const;
    void *                  MapBuffer(vk_buffer_s& buffer) const;
    bool                    UnmapBuffer(vk_buffer_s& buffer) const;

    bool                    CreateBufferAddInitData(vk_buffer_s& buffer, VkBufferUsageFlags usage_flags, const void* data, size_t data_size, bool staging);
    bool                    CreateVertexBuffer(vk_buffer_s & buffer, const void * data, size_t data_size, bool staging);
    bool                    CreateIndexBuffer(vk_buffer_s & buffer, const void* data, size_t data_size, bool staging);

    bool                    CreateSampler(VkFilter mag_filter, VkFilter min_filter, VkSamplerMipmapMode mipmap_mode, 
                                VkSamplerAddressMode address_mode, VkSampler& sampler);
    void                    DestroySampler(VkSampler & sampler);
    
    bool                    Create2DImage(vk_image_s & vk_image, VkImageCreateFlags flags, VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t array_layers,
                                VkMemoryPropertyFlags memory_property_flags,
                                VkImageAspectFlags image_aspect_flags,
                                VkImageViewType image_view_type,
                                VkSampler sampler,
                                VkImageLayout image_layout);
    void                    DestroyImage(vk_image_s& vk_image);

    bool					Load2DTexture(const char* filename, 
                                VkFormat format, VkImageUsageFlags image_usage, 
                                VkSampler sampler, VkImageLayout image_layout, vk_image_s & vk_image);
    bool                    Update2DTexture(const image_s & pic, vk_image_s& vk_image);

    bool                    LoadCubeMaps(const char* filename,
                                VkFormat format, VkImageUsageFlags image_usage,
                                VkSampler sampler, VkImageLayout image_layout, vk_image_s& vk_image);

    void                    DestroyFramebuffer(VkFramebuffer & vk_framebuffer);
    void                    DestroyRenderPass(VkRenderPass & vk_render_pass);

protected:

    enum class camera_mode_t {
        CM_MOVE_CAMERA,
        CM_ROTATE_OBJECT
    };

    static const uint32_t   ROTATION_YAW_BIT = 1;
    static const uint32_t   ROTATION_PITCH_BIT = 2;
    
    camera_mode_t           camera_mode_;
    uint32_t                camera_rotation_flags_;
    float                   z_near_;
    float                   z_far_;
    float                   fovy_;  // in degree

    // configuration
    uint32_t                cfg_viewport_cx_;
    uint32_t                cfg_viewport_cy_;

#if defined(_WIN32)
    const TCHAR *           cfg_demo_win_class_name_;

	HINSTANCE				hInstance_;
	HWND					hWnd_;
#endif

	// SPIR-V shader file location
	char					shaders_dir_[MAX_PATH];

    char                    textures_dir_[MAX_PATH];

    char                    models_dir_[MAX_PATH];

#ifdef _DEBUG
	// for validation
	PFN_vkCreateDebugReportCallbackEXT	vkCreateDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT			vkDebugReportMessageEXT;
	PFN_vkDestroyDebugReportCallbackEXT	vkDestroyDebugReportCallbackEXT;
	VkDebugReportCallbackEXT			debug_report_callback_;
#endif

	// vk
	VkInstance				vk_instance_;

	// physical device
	VkPhysicalDevice        vk_physical_device_;

	VkPhysicalDeviceMemoryProperties	vk_physical_device_memory_properties_;	// store memory parameters (number of heaps, their size, and types)
	VkPhysicalDeviceProperties2         vk_physical_device_properties2_;
	VkPhysicalDeviceMeshShaderPropertiesEXT vk_physical_device_mesh_shader_propertices_;
	VkPhysicalDeviceFeatures			vk_physical_device_features_;
    // VkPhysicalDeviceFeatures2           vk_physical_device_features2_;
    uint32_t                            vk_physical_device_graphics_queue_family_index_;

    VkFormat                vk_depth_format_;

    // logic device
    void *                  vk_device_create_next_chain_;   // fill next pointer
    VkDevice                vk_device_;
    VkQueue                 vk_graphics_queue_; // graphics queue
    

    // Semaphore:  GPU, GPU syncronization
    VkSemaphore             vk_semaphore_present_complete_; // swap chain image presentation
    VkSemaphore             vk_semaphore_render_complete_;  // command buffer sumission and execution

    // surface
    VkSurfaceKHR            vk_surface_;

    // swapchain
    VkSwapchainKHR          vk_swapchain_;
    VkFormat                vk_swapchain_color_format_;

    struct swapchain_image_s {
        VkImage             image_;
        VkImageView         image_view_;
    };

    static constexpr int    MAX_SWAPCHAIN_IMAGES = 16;
    swapchain_image_s       vk_swapchain_images_[MAX_SWAPCHAIN_IMAGES];
    int                     vk_swapchain_image_count_;

    // fence: CPU, GPU syncronization
    VkFence                 vk_wait_fences_[MAX_SWAPCHAIN_IMAGES];
    int                     vk_wait_fence_count_;

    // depth stencil

    struct depth_stencil_s {
        VkDeviceMemory      memory_;
        VkImage             image_;
        VkImageView         image_view_;
    };

    depth_stencil_s         vk_depth_stencil_;

    // renderpass
    VkRenderPass            vk_render_pass_;

    // framebuffer
    VkFramebuffer           vk_framebuffers_[MAX_SWAPCHAIN_IMAGES];
    int                     vk_framebuffer_count_;

    // command pool
    VkCommandPool           vk_command_pool_;
    VkCommandPool           vk_command_pool_transient_;

    // command buffers
    VkCommandBuffer         vk_draw_cmd_buffers_[MAX_SWAPCHAIN_IMAGES];
    int                     vk_draw_cmd_buffer_count_;

    // descriptor
    VkDescriptorPool        vk_descriptor_pool_;
    // pipeline cache
    VkPipelineCache         vk_pipeline_cache_;

    bool                    enable_display_;

    virtual void            AddAdditionalInstanceExtensions(std::vector<const char*> & extensions) const;
    virtual void            AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) const;

    void                    SubmitCommandBufferAndWait(VkCommandBuffer command_buffer);

    // helper
    uint32_t                GetMemoryTypeIndex(uint32_t memory_type_bits, 
                                VkMemoryPropertyFlags required_memory_properties) const;
    bool                    LoadShader(const char* filename, VkShaderModule& shader_module) const;
    
    void                    SetTitle(const char * text);
    void                    GetProjMatrix(glm::mat4& proj_mat) const;
    void                    GetViewMatrix(glm::mat4 & view_mat) const;
    void                    GetModelMatrix(glm::mat4& model_mat) const;

    VkFormat                GetIdealDepthFormat() const;
    VkFormat                GetIdealDepthStencilFormat() const;

    // flags
    static const uint32_t   VERTEX_FIELD_COLOR = 1;
    static const uint32_t   VERTEX_FIELD_NORMAL = 2;
    static const uint32_t   VERTEX_FIELD_UV = 4;
    static const uint32_t   VERTEX_FIELD_TANGENT = 8;
    static const uint32_t   VERTEX_FIELD_ALL = 0xffffffff;

    struct create_pipeline_vert_frag_params_s {
        const char*         vertex_shader_filename_;
        const char*         framgment_shader_filename_;
        vertex_format_t     vertex_format_;
        instance_format_t   instance_format_;
        uint32_t            additional_vertex_fields_;
        VkPrimitiveTopology primitive_topology_;
        VkBool32            primitive_restart_enable_;
        VkPolygonMode       polygon_mode_;
        VkCullModeFlags     cull_mode_;
        VkFrontFace         front_face_;
        VkBool32            depth_bias_enable_;
        float               depth_bias_constant_factor_;
        float               depth_bias_slope_factor_;
        VkBool32            depth_test_enable_;
        VkBool32            depth_write_enable_;
        VkPipelineLayout    pipeline_layout_;
        VkRenderPass        render_pass_;
    };

    bool                    CreatePipelineVertFrag(const create_pipeline_vert_frag_params_s& params,
                                VkPipeline& pipeline);

protected:

    struct camera_s {
        glm::vec3           pos_;
        glm::vec3           target_;
        glm::vec3           up_;
    } camera_;

    struct movement_s {
        int                 forward_;   // 0: stopped, 1: forward, -1: backward
        int                 right_;     // 0: stopped, 1: right, -1: left
    } movement_;

    static const int        ANGLE_YAW = 0;
    static const int        ANGLE_PITCH = 1;
    // TODO: support ROLL

    float                   view_angles_[2];    // measured in degree
    float                   model_scale_;

    float                   move_speed_;

    glm::mat4 *             model_rotate_mat_;

    void                    CursorRotate(float delta_yaw, float delta_pitch);

    virtual void            FuncKeyDown(uint32_t key);

    // helper

    void                    DestroyDescriptorSetLayout(VkDescriptorSetLayout & descriptor_set_layout);

    void                    DestroyPipelineLayout(VkPipelineLayout & pipeline_layout);
    void                    DestroyPipeline(VkPipeline & pipeline);

private:

    bool                    left_button_down_;
    int                     cursor_x_;
    int                     cursor_y_;
    float                   mouse_sensitive_;

    bool                    CreateDemoWindow();

#if defined(_WIN32)
    LRESULT                 DemoWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    // vulkan instance
    bool                    CreateVkInstance();
    void                    DestroyVkInstance();

#ifdef _DEBUG
    VkBool32                DemoDebugReportCallback(const char* pMessage);
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage, void* pUserData);
#endif

    // physical device
    bool                    SelectPhysicalDevice();

    // logic device
    bool                    CreateDevice();
    void                    DestroyDevice();

    // semaphores used in this demo
    bool                    CreateDemoSemaphores();
    void                    DestroyDemoSemaphores();

    // surface
    bool                    CreateSurface();
    void                    DestroySurface();

    // swapchain
    bool                    CreateSwapChain(bool vsync);
    void                    DestroySwapChain();

    // fences used in thie demo
    bool                    CreateDemoFences();
    void                    DestroyDemoFences();

    // depth stencil
    bool                    CreateDepthStencil();
    void                    DestroyDepthStencil();

    // render pass
    bool                    CreateRenderPass();

    // framebuffers
    bool                    CreateFramebuffers();
    void                    DestroyFramebuffers();

    // command pool
    bool                    CreateCommandPools();
    void                    DestroyCommandPools();

    // command buffers
    bool                    AllocCommandBuffers();
    void                    FreeCommandBuffers();

    // pipeline
    bool                    CreatePipelineCache();
    void                    DestroyPipelineCache();

    // descriptor set pool
    bool					CreateDescriptorPools(
                                uint32_t max_uniform_buffer,
                                uint32_t max_storage_buffer,
                                uint32_t max_texture,
                                uint32_t max_desp_set);
    void					DestroyDescriptorPools();

    void                    CheckMovement();
    
    void                    OnResize();
};
