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

	bool					Init(const char* project_shader_dir);
	void					Shutdown();
    virtual void			Display() = 0;
    virtual void			BuildCommandBuffers() = 0;

    void                    MainLoop();

    // interface
    bool                    LoadModel(const char * filename, bool move_to_origin, model_s & model) const;

    bool                    CreateBuffer(vk_buffer_s& buffer, VkBufferUsageFlags usage, VkDeviceSize req_size) const;
    void                    DestroyBuffer(vk_buffer_s& buffer) const;
    void					UpdateBuffer(vk_buffer_s& buffer, const void* host_data, size_t host_data_size) const;

    void *                  MapMemory(const vk_buffer_s& buffer) const;
    void                    UnmapMemory(const vk_buffer_s& buffer);

protected:

    enum class camera_mode_t {
        CM_MOVE_CAMERA,
        CM_ROTATE_OBJECT
    };

    camera_mode_t           camera_mode_;

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

    // descriptor set pool
    bool					CreateDescriptorPools(
                                uint32_t max_uniform_buffer, 
                                uint32_t max_storage_buffer, 
                                uint32_t max_texture,
                                uint32_t max_desp_set);
    void					DestroyDescriptorPools();

    // helper
    uint32_t                GetMemoryTypeIndex(uint32_t memory_type_bits, 
                                VkMemoryPropertyFlags required_memory_properties) const;
    bool                    LoadShader(const char* filename, VkShaderModule& shader_module) const;
    
    void                    SetTitle(const char * text);
    void                    GetViewMatrix(glm::mat4 & view_mat) const;
    virtual void            GetModelMatrix(glm::mat4& model_mat) const;

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

    void                    RotateCamera();

    virtual void            KeyF2Down();

    // helper
    

    bool                    Create2DImage(vk_image_s & vk_image, VkFormat format, VkImageTiling tiling, 
                                VkImageUsageFlags usage, uint32_t width, uint32_t height,
                                VkMemoryPropertyFlags memory_property_flags,
                                VkImageAspectFlags image_aspect_flags);
    void                    Destroy2DImage(vk_image_s& vk_image);

    bool					Load2DTexture(const char* filename, VkFormat format, VkImageUsageFlags image_usage, vk_image_s & vk_image);

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

    // init supported depth format
    bool                    InitSupportedDepthFormat();

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
    void                    DestroyRenderPass();

    // framebuffers
    bool                    CreateFramebuffers();
    void                    DestroyFramebuffers();

    // command pool
    bool                    CreateCommandPool();
    void                    DestroyCommandPool();

    // command buffers
    bool                    AllocCommandBuffers();
    void                    FreeCommandBuffers();

    // pipeline
    bool                    CreatePipelineCache();
    void                    DestroyPipelineCache();

    void                    CheckMovement();
    
    void                    OnResize();
};
