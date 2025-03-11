/******************************************************************************
 Demo base class
 *****************************************************************************/

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include <vector>
#include <array>

// vulkan

#if defined(_WIN32)
# define VK_USE_PLATFORM_WIN32_KHR		// to use VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#include <vulkan/vulkan.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
================================================================================
VkDemo
================================================================================
*/
class VkDemo {
public:
	VkDemo();
	virtual ~VkDemo();

	bool					Init(const char* project_shader_dir);
	void					Shutdown();
    virtual void			Display() = 0;

    void                    MainLoop();

protected:

    // configuration
    uint32_t                cfg_viewport_cx_;
    uint32_t                cfg_viewport_cy_;

#if defined(_WIN32)
    const TCHAR *           cfg_demo_win_class_name_;

	HINSTANCE				hInstance_;
	HWND					hWnd_;
#endif

	// SPIR-V shader file location
	char					shader_dir_[1024];

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

	std::vector<VkQueueFamilyProperties>    vk_physical_device_queue_family_properties_;
	VkPhysicalDeviceMemoryProperties	vk_physical_device_memory_properties_;	// store memory parameters (number of heaps, their size, and types)
	VkPhysicalDeviceProperties2         vk_physical_device_properties2_;
	VkPhysicalDeviceMeshShaderPropertiesEXT vk_physical_device_mesh_shader_propertices_;
	VkPhysicalDeviceFeatures			vk_physical_device_features_;
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

    std::vector<swapchain_image_s> vk_swapchain_images_;

    // fence: CPU, GPU syncronization
    std::vector<VkFence>    vk_wait_fences_;

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
    std::vector<VkFramebuffer>  vk_framebuffers_;

    // command pool
    VkCommandPool           vk_command_pool_;

    // command buffers
    std::vector<VkCommandBuffer>    vk_draw_cmd_buffers_;

    bool                    enable_display_;

    virtual void            AddAdditionalInstanceExtensions(std::vector<const char*> & extensions);
    virtual void            AddAdditionalDeviceExtensions(std::vector<const char*>& extensions);

    // helper
    uint32_t                GetMemoryTypeIndex(uint32_t memory_type_bits, VkMemoryPropertyFlags required_memory_properties);
    bool                    LoadShader(const char* filename, VkShaderModule& shader_module);

private:

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
};
