/******************************************************************************
 Demo base class - implementation
 *****************************************************************************/

#include "inc.h"

#if defined(_WIN32)
# include <windowsx.h>
# pragma comment(lib, "vulkan-1.lib")
#endif

/*
================================================================================
VkDemo
================================================================================
*/
VkDemo::VkDemo():
    camera_mode_(camera_mode_t::CM_MOVE_CAMERA),
    cfg_viewport_cx_(640),
    cfg_viewport_cy_(480),

#if defined(_WIN32)
    cfg_demo_win_class_name_(TEXT("Vulkan Demo")),
	hInstance_(NULL),
	hWnd_(NULL),
#endif

#ifdef _DEBUG
	vkCreateDebugReportCallbackEXT(nullptr),
	vkDebugReportMessageEXT(nullptr),
	vkDestroyDebugReportCallbackEXT(nullptr),
	debug_report_callback_(nullptr),	// callback object handle
#endif

	vk_instance_(VK_NULL_HANDLE),
	vk_physical_device_(VK_NULL_HANDLE),
	vk_physical_device_graphics_queue_family_index_(-1),
    vk_depth_format_(VK_FORMAT_UNDEFINED),
	vk_device_(0),
    vk_device_create_next_chain_(nullptr),
    vk_graphics_queue_(VK_NULL_HANDLE),
    vk_semaphore_present_complete_(VK_NULL_HANDLE),
    vk_semaphore_render_complete_(VK_NULL_HANDLE),
    vk_surface_(VK_NULL_HANDLE),
	vk_swapchain_(VK_NULL_HANDLE),
	vk_swapchain_color_format_(VK_FORMAT_UNDEFINED),
    vk_swapchain_image_count_(0),
    vk_wait_fence_count_(0),
    vk_framebuffer_count_(0),
    vk_draw_cmd_buffer_count_(0),
    vk_descriptor_pool_(VK_NULL_HANDLE),
    vk_pipeline_cache_(VK_NULL_HANDLE),
    vk_render_pass_(VK_NULL_HANDLE),
	vk_command_pool_(VK_NULL_HANDLE),
    enable_display_(false),
    model_scale_(1.0f),
    move_speed_(2.0f),
    left_button_down_(false),
    cursor_x_(0),
    cursor_y_(0),
    mouse_sensitive_(0.5f)
{
	shaders_dir_[0] = '\0';
    textures_dir_[0] = '\0';
    models_dir_[0] = '\0';

	memset(&vk_physical_device_memory_properties_, 0, sizeof(vk_physical_device_memory_properties_));
	memset(&vk_physical_device_properties2_, 0, sizeof(vk_physical_device_properties2_));
	memset(&vk_physical_device_mesh_shader_propertices_, 0, sizeof(vk_physical_device_mesh_shader_propertices_));
	memset(&vk_physical_device_features_, 0, sizeof(vk_physical_device_features_));

	memset(&vk_depth_stencil_, 0, sizeof(vk_depth_stencil_));

    // z up

    /*
     z   y
     |  /
     | /
     |/
      ------x
     */
    camera_.pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
    camera_.target_ = glm::vec3(0.0f, 1.0f, 0.0f);
    camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

    memset(&movement_, 0, sizeof(movement_));

    view_angles_[ANGLE_YAW] = 0.0f;
    view_angles_[ANGLE_PITCH] = 0.0f;
}

VkDemo::~VkDemo() {
	// do nothing
}

bool VkDemo::Init(const char* project_shader_dir) {
    // setup folders
    Str_SPrintf(shaders_dir_, MAX_PATH, "%s/shaders/%s",
        GetDataFolder(), project_shader_dir);
    
    Str_SPrintf(textures_dir_, MAX_PATH, "%s/textures",
        GetDataFolder());

    Str_SPrintf(models_dir_, MAX_PATH, "%s/models",
        GetDataFolder());

    if (!CreateDemoWindow()) {
        return false;
    }

    if (!CreateVkInstance()) {
        return false;
    }

    if (!SelectPhysicalDevice()) {
        return false;
    }

    if (!InitSupportedDepthFormat()) {
        return false;
    }

    if (!CreateDevice()) {
        return false;
    }

    if (!CreateDemoSemaphores()) {
        return false;
    }

    if (!CreateSurface()) {
        return false;
    }

    if (!CreateSwapChain(false)) {
        return false;
    }

    if (!CreateDemoFences()) {
        return false;
    }

    if (!CreateDepthStencil()) {
        return false;
    }

    if (!CreateRenderPass()) {
        return false;
    }

    if (!CreateFramebuffers()) {
        return false;
    }

    if (!CreateCommandPool()) {
        return false;
    }

    if (!AllocCommandBuffers()) {
        return false;
    }

    if (!CreatePipelineCache()) {
        return false;
    }

    return true;
}

void VkDemo::Shutdown() {
    DestroyPipelineCache();
    FreeCommandBuffers();
    DestroyCommandPool();
    DestroyFramebuffers();
    DestroyRenderPass();
    DestroyDepthStencil();
    DestroyDemoFences();
    DestroySwapChain();
    DestroySurface();
    DestroyDemoSemaphores();
    DestroyDevice();
    DestroyVkInstance();
}

void VkDemo::MainLoop() {
#if defined(_WIN32)

    Display();  // first draw

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

// interface
bool VkDemo::LoadModel(const char* filename, bool move_to_origin, model_s& model) const {
    char full_filename[MAX_PATH];
    Str_SPrintf(full_filename, MAX_PATH, "%s/%s", models_dir_, filename);

    return Model_Load(full_filename, move_to_origin, model);
}

bool VkDemo::CreateBuffer(vk_buffer_s& buffer, VkBufferUsageFlags usage, VkDeviceSize req_size) const {
    // The Vulkan spec states: If size is not equal to VK_WHOLE_SIZE, size must either be a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize, 
        // or offset plus size must equal the size of memory
    VkDeviceSize nonCoherentAtomSize = vk_physical_device_properties2_.properties.limits.nonCoherentAtomSize;
    VkDeviceSize aligned_req_size = req_size;

    if (aligned_req_size % nonCoherentAtomSize) {
        aligned_req_size = ((aligned_req_size / nonCoherentAtomSize) + 1) * nonCoherentAtomSize;
    }

    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = aligned_req_size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr  // is a pointer to an array of queue families that will access this buffer.
        // It is ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
    };

    if (VK_SUCCESS != vkCreateBuffer(vk_device_, &create_info, nullptr, &buffer.buffer_)) {
        return false;
    }

    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(vk_device_, buffer.buffer_, &mem_req);

    VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = GetMemoryTypeIndex(mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (VK_SUCCESS != vkAllocateMemory(vk_device_, &mem_alloc_info, nullptr, &buffer.memory_)) {
        return false;
    }

    buffer.memory_size_ = mem_req.size;

    if (VK_SUCCESS != vkBindBufferMemory(vk_device_, buffer.buffer_, buffer.memory_, 0)) {
        return false;
    }

    buffer.buffer_info_.buffer = buffer.buffer_;
    buffer.buffer_info_.offset = 0;
    buffer.buffer_info_.range = aligned_req_size;

    return true;
}

void VkDemo::DestroyBuffer(vk_buffer_s& buffer) const {
    if (buffer.memory_) {
        vkFreeMemory(vk_device_, buffer.memory_, nullptr);
        buffer.memory_ = VK_NULL_HANDLE;
    }

    if (buffer.buffer_) {
        vkDestroyBuffer(vk_device_, buffer.buffer_, nullptr);
        buffer.buffer_ = VK_NULL_HANDLE;
    }

    memset(&buffer, 0, sizeof(buffer));
}

void VkDemo::UpdateBuffer(vk_buffer_s& buffer, const void* host_data, size_t host_data_size) const {
    void* data = nullptr;
    VkResult rt = vkMapMemory(vk_device_, buffer.memory_, buffer.buffer_info_.offset, buffer.buffer_info_.range, 0, &data);
    if (rt != VK_SUCCESS) {
        printf("[UpdateBuffer] vkMapMemory error\n");
        return;
    }

    memcpy(data, host_data, host_data_size);

    // flush to make change visible to device
    VkMappedMemoryRange mapped_range = {};

    mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_range.pNext = nullptr;
    mapped_range.memory = buffer.memory_;
    mapped_range.offset = buffer.buffer_info_.offset;
    mapped_range.size = buffer.buffer_info_.range;

    rt = vkFlushMappedMemoryRanges(vk_device_, 1, &mapped_range);
    if (rt != VK_SUCCESS) {
        printf("[UpdateBuffer] vkFlushMappedMemoryRanges error\n");
    }

    vkUnmapMemory(vk_device_, buffer.memory_);
}

void* VkDemo::MapMemory(const vk_buffer_s& buffer) const {
    void* data = nullptr;
    if (VK_SUCCESS != vkMapMemory(vk_device_, buffer.memory_, 0, buffer.memory_size_, 0, &data)) {
        return nullptr;
    }

    return data;
}

void VkDemo::UnmapMemory(const vk_buffer_s& buffer) {
    vkUnmapMemory(vk_device_, buffer.memory_);
}

void VkDemo::AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) const {
    // to override by child class
}

void VkDemo::AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) const {
    // to override by child class
}

// descriptor set pool
bool VkDemo::CreateDescriptorPools(
    uint32_t max_uniform_buffer, uint32_t max_storage_buffer, uint32_t max_texture, uint32_t max_desp_set)
{
    VkDescriptorPoolCreateInfo create_info = {};

    std::vector<VkDescriptorPoolSize> pool_size_array;

    if (max_uniform_buffer) {
        pool_size_array.push_back({
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = max_uniform_buffer
        });
    }

    if (max_storage_buffer) {
        pool_size_array.push_back({
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = max_storage_buffer
            });
    }

    if (max_texture) {
        pool_size_array.push_back({
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = max_texture
            });
    }

    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    create_info.maxSets = max_desp_set;    // is the maximum number of descriptor sets that can be allocated from the pool.
    create_info.poolSizeCount = (uint32_t)pool_size_array.size();
    create_info.pPoolSizes = pool_size_array.data();

    return VK_SUCCESS == vkCreateDescriptorPool(vk_device_, &create_info, nullptr, &vk_descriptor_pool_);

}

void VkDemo::DestroyDescriptorPools() {
    if (vk_descriptor_pool_) {
        vkDestroyDescriptorPool(vk_device_, vk_descriptor_pool_, nullptr);
        vk_descriptor_pool_ = VK_NULL_HANDLE;
    }
}

// helper
uint32_t VkDemo::GetMemoryTypeIndex(uint32_t memory_type_bits, 
    VkMemoryPropertyFlags required_memory_properties) const
{
    uint32_t bit_mask = 1;
    for (uint32_t i = 0; i < vk_physical_device_memory_properties_.memoryTypeCount; ++i) {
        if (memory_type_bits & bit_mask) {
            // support i's memory type
            if ((vk_physical_device_memory_properties_.memoryTypes[i].propertyFlags & required_memory_properties)
                == required_memory_properties) {
                return i;
            }
        }
        bit_mask <<= 1;
    }

    printf("Could not found memory type index match the request\n");
    return 0xFFFFFFFF;
}

bool VkDemo::LoadShader(const char* filename, VkShaderModule& shader_module) const {
    shader_module = VK_NULL_HANDLE;

    char fullfilename[1024];
    sprintf_s(fullfilename, "%s/%s", shaders_dir_, filename);

    void* code = nullptr;
    int32_t code_len = 0;
    if (!File_LoadBinary32(fullfilename, code, code_len)) {
        return false;
    }

    VkShaderModuleCreateInfo create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.codeSize = (size_t)code_len;
    create_info.pCode = (const uint32_t*)code;

    VkResult rt = vkCreateShaderModule(vk_device_, &create_info, nullptr, &shader_module);

    File_FreeBinary(code);

    return rt == VK_SUCCESS;
}

void VkDemo::SetTitle(const char* text) {
#if defined(_WIN32)
    char16_t buf[1024];
    Str_UTF8ToUTF16(text, buf, 1024);

    SetWindowText(hWnd_, (LPCTSTR)buf);
#endif
}

void VkDemo::GetViewMatrix(glm::mat4& view_mat) const {
    view_mat = glm::lookAt(camera_.pos_, camera_.target_, camera_.up_);
}

void VkDemo::GetModelMatrix(glm::mat4& model_mat) const {
    if (camera_mode_ == camera_mode_t::CM_MOVE_CAMERA) {
        model_mat = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale_));
    }
    else {

        model_mat = glm::scale(glm::mat4(1.0f),
            glm::vec3(model_scale_));

        model_mat = glm::rotate(model_mat,
            glm::radians(-view_angles_[ANGLE_PITCH]),
            glm::vec3(1.0f, 0.0f, 0.0f));

        model_mat = glm::rotate(model_mat,
            glm::radians(view_angles_[ANGLE_YAW]),
            glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

bool VkDemo::CreateDemoWindow() {
#if defined(_WIN32)
    WNDCLASSEX wc = {};

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc; 
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hInstance = hInstance_;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = NULL;    // no menu
    wc.lpszClassName = cfg_demo_win_class_name_;
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wc)) {
        return NULL;
    }

    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT wnd_rect;
    wnd_rect.left = 0;
    wnd_rect.top = 0;
    wnd_rect.right = cfg_viewport_cx_;
    wnd_rect.bottom = cfg_viewport_cy_;

    AdjustWindowRectEx(&wnd_rect, dwStyle, FALSE /* bMenu */, dwExStyle);

    int wnd_cx = wnd_rect.right - wnd_rect.left;
    int wnd_cy = wnd_rect.bottom - wnd_rect.top;

    int screen_cx = GetSystemMetrics(SM_CXSCREEN);
    int screen_cy = GetSystemMetrics(SM_CYSCREEN);

    int wnd_x = (screen_cx - wnd_cx) / 2;
    int wnd_y = (screen_cy - wnd_cy) / 2;

    HWND hWnd = CreateWindowEx(dwExStyle, cfg_demo_win_class_name_, cfg_demo_win_class_name_, dwStyle,
        wnd_x, wnd_y, wnd_cx, wnd_cy, NULL /* hWndParent */, NULL /*hMenu*/, hInstance_, NULL /*lpParam*/);

    if (hWnd) {
        hWnd_ = hWnd;

        SetWindowLongPtr(hWnd, 0, (LONG_PTR)this);

        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
    }

    return hWnd != NULL;
#else
    return false;   // TODO: other platform
#endif
}

void VkDemo::RotateCamera() {
    if (camera_mode_ == camera_mode_t::CM_MOVE_CAMERA) {

        // normalize yaw in range [0, 360)
        while (view_angles_[ANGLE_YAW] >= 360.0f) {
            view_angles_[ANGLE_YAW] -= 360.0f;
        }

        while (view_angles_[ANGLE_YAW] < 0.0f) {
            view_angles_[ANGLE_YAW] += 360.0f;
        }

        // clamp pitch to avoid gimbal lock
        if (view_angles_[ANGLE_PITCH] > 89.0f) {
            view_angles_[ANGLE_PITCH] = 89.0f;
        }

        if (view_angles_[ANGLE_PITCH] < -89.0f) {
            view_angles_[ANGLE_PITCH] = -89.0f;
        }

        glm::vec4 forward4 = glm::rotate(glm::mat4(1.0),
            glm::radians(view_angles_[ANGLE_YAW]), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        glm::vec3 right = glm::cross(glm::vec3(forward4), camera_.up_);

        forward4 = glm::rotate(glm::mat4(1.0),
            glm::radians(view_angles_[ANGLE_PITCH]), right) * forward4;

        camera_.target_ = camera_.pos_ + glm::vec3(forward4);

    }
}

void VkDemo::KeyF2Down() {
    // to override
}

// helper

bool VkDemo::Create2DImage(vk_image_s& vk_image, VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, uint32_t width, uint32_t height, 
    VkMemoryPropertyFlags memory_property_flags,
    VkImageAspectFlags image_aspect_flags) 
{
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (VK_SUCCESS != vkCreateImage(vk_device_, &image_create_info, nullptr, &vk_image.image_)) {
        return false;
    }

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(vk_device_, vk_image.image_, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = GetMemoryTypeIndex(
            memory_requirements.memoryTypeBits,
            memory_property_flags)
    };

    if (VK_SUCCESS != vkAllocateMemory(vk_device_, &memory_allocate_info, nullptr, &vk_image.memory_)) {
        return false;
    }

    if (VK_SUCCESS != vkBindImageMemory(vk_device_, vk_image.image_, vk_image.memory_, 0 /* memoryOffset */)) {
        return false;
    }

    vk_image.memory_size_ = memory_requirements.size;

    // create image view
    VkImageViewCreateInfo image_view_create_info = {};

    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = vk_image.image_;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    // components
    image_view_create_info.subresourceRange.aspectMask = image_aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    if (VK_SUCCESS != vkCreateImageView(vk_device_, &image_view_create_info,
        nullptr, &vk_image.image_view_)) {
        return false;
    }

    return true;
}

void VkDemo::Destroy2DImage(vk_image_s& vk_image) {
    if (vk_image.image_view_) {
        vkDestroyImageView(vk_device_, vk_image.image_view_, nullptr);
        vk_image.image_view_ = VK_NULL_HANDLE;
    }

    if (vk_image.memory_) {
        vkFreeMemory(vk_device_, vk_image.memory_, nullptr);
        vk_image.memory_ = VK_NULL_HANDLE;
    }

    if (vk_image.image_) {
        vkDestroyImage(vk_device_, vk_image.image_, nullptr);
        vk_image.image_ = VK_NULL_HANDLE;
    }
}

bool VkDemo::Load2DTexture(const char* filename, VkFormat format, VkImageUsageFlags image_usage, vk_image_s& vk_image) {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_properties);

    if (!(format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        printf("linearTilingFeatures not support VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT\n");
        return false;
    }

    char full_filename[MAX_PATH];
    Str_SPrintf(full_filename, MAX_PATH, "%s/%s", textures_dir_, filename);

    image_s pic = {};
    if (!Img_Load(full_filename, pic)) {
        printf("Failed to load texture \"%s\"\n", full_filename);
        return false;
    }

    // VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation-dependent arrangement, 
    //                          for more efficient memory access

    // VK_IMAGE_TILING_LINEAR: specifies linear tiling (texels are laid out in memory in row-major order, 
    //                         possibly with some padding on each row).
    if (!Create2DImage(vk_image, format, VK_IMAGE_TILING_LINEAR, image_usage,
        (uint32_t)pic.width_, (uint32_t)pic.height_,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT)) 
    {
        Img_Free(pic);
        return false;
    }

    // copy contents
    void* mapped = nullptr;
    if (VK_SUCCESS != vkMapMemory(vk_device_, vk_image.memory_, 0, vk_image.memory_size_, 0 /* flags */, &mapped)) {
        Img_Free(pic);
        return false;
    }

    memcpy(mapped, pic.pixels_, pic.width_ * pic.height_ * 4);

    vkUnmapMemory(vk_device_, vk_image.memory_);

    Img_Free(pic);

    // change layout from VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkCommandBuffer cmd_buffer_load_tex = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};

    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.pNext = nullptr;
    cmd_buffer_alloc_info.commandPool = vk_command_pool_;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = 1;

    if (VK_SUCCESS != vkAllocateCommandBuffers(vk_device_, &cmd_buffer_alloc_info, &cmd_buffer_load_tex)) {
        return false;
    }

    VkCommandBufferBeginInfo cmdbuf_begin_info = {};

    cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbuf_begin_info.pNext = nullptr;
    cmdbuf_begin_info.flags = 0;
    cmdbuf_begin_info.pInheritanceInfo = nullptr;

    if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buffer_load_tex, &cmdbuf_begin_info)) {
        vkFreeCommandBuffers(vk_device_, vk_command_pool_, 1, &cmd_buffer_load_tex);
        return false;
    }

    VkImageMemoryBarrier image_memory_barrier = {};

    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// wrong: VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = vk_image.image_;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd_buffer_load_tex,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0 /* dependencyFlags */,
        0 /* memoryBarrierCount */, nullptr /* pMemoryBarriers */,
        0 /* bufferMemoryBarrierCount */, nullptr /* pBufferMemoryBarries */,
        1, &image_memory_barrier);

    vkEndCommandBuffer(cmd_buffer_load_tex);

    VkFence temp_fence = VK_NULL_HANDLE;

    VkFenceCreateInfo fence_create_info = {};

    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = 0;

    if (VK_SUCCESS != vkCreateFence(vk_device_, &fence_create_info, nullptr, &temp_fence)) {
        vkFreeCommandBuffers(vk_device_, vk_command_pool_, 1, &cmd_buffer_load_tex);
        return false;
    }

    VkSubmitInfo queue_submit_info = {};

    queue_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queue_submit_info.pNext = nullptr;
    queue_submit_info.waitSemaphoreCount = 0;
    queue_submit_info.pWaitSemaphores = nullptr;
    queue_submit_info.pWaitDstStageMask = nullptr;
    queue_submit_info.commandBufferCount = 1;
    queue_submit_info.pCommandBuffers = &cmd_buffer_load_tex;
    queue_submit_info.signalSemaphoreCount = 0;
    queue_submit_info.pSignalSemaphores = nullptr;

    if (VK_SUCCESS != vkQueueSubmit(vk_graphics_queue_, 1, &queue_submit_info, temp_fence)) {
        vkDestroyFence(vk_device_, temp_fence, nullptr);
        vkFreeCommandBuffers(vk_device_, vk_command_pool_, 1, &cmd_buffer_load_tex);
        return false;
    }

    if (VK_SUCCESS != vkWaitForFences(vk_device_, 1, &temp_fence, VK_TRUE, UINT64_MAX)) {
        vkDestroyFence(vk_device_, temp_fence, nullptr);
        vkFreeCommandBuffers(vk_device_, vk_command_pool_, 1, &cmd_buffer_load_tex);
        return false;
    }

    vkDestroyFence(vk_device_, temp_fence, nullptr);
    vkFreeCommandBuffers(vk_device_, vk_command_pool_, 1, &cmd_buffer_load_tex);

    return true;
}

void VkDemo::DestroyDescriptorSetLayout(VkDescriptorSetLayout& descriptor_set_layout) {
    if (descriptor_set_layout) {
        vkDestroyDescriptorSetLayout(vk_device_, descriptor_set_layout, nullptr);
        descriptor_set_layout = VK_NULL_HANDLE;
    }
}

void VkDemo::DestroyPipelineLayout(VkPipelineLayout& pipeline_layout) {
    if (pipeline_layout) {
        vkDestroyPipelineLayout(vk_device_, pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
    }
}

void VkDemo::DestroyPipeline(VkPipeline& pipeline) {
    if (pipeline) {
        vkDestroyPipeline(vk_device_, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
}

#if defined(_WIN32)
LRESULT VkDemo::DemoWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
# define KEY_W  0x57
# define KEY_S  0x53
# define KEY_A  0x41
# define KEY_D  0x44

    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        enable_display_ = false;
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        if (enable_display_) {
            Display();
        }
        break;
    case WM_SIZE:
        OnResize();
        break;
    case WM_LBUTTONDOWN:
        left_button_down_ = true;
        cursor_x_ = GET_X_LPARAM(lParam);
        cursor_y_ = GET_Y_LPARAM(lParam);
        break;
    case WM_LBUTTONUP:
        left_button_down_ = false;
        break;
    case WM_MOUSEMOVE:
        if (left_button_down_) {
            // hold left button to rotate the camera

            int new_cursor_x = GET_X_LPARAM(lParam);
            int new_cursor_y = GET_Y_LPARAM(lParam);

            int delta_x = new_cursor_x - cursor_x_;
            int delta_y = -(new_cursor_y - cursor_y_);

            float delta_yaw = (float)delta_x;
            float delta_pitch = (float)delta_y;

            view_angles_[ANGLE_YAW] += delta_yaw * 0.5f;
            view_angles_[ANGLE_PITCH] += delta_pitch * 0.5f;

            RotateCamera();

            cursor_x_ = new_cursor_x;
            cursor_y_ = new_cursor_y;

            // update window
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_F2:
            KeyF2Down();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        case KEY_W:
            movement_.forward_ = 1;
            break;
        case KEY_S:
            movement_.forward_ = -1;
            break;
        case KEY_A:
            movement_.right_ = -1;
            break;
        case KEY_D:
            movement_.right_ = 1;
            break;
        }
        CheckMovement();
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    case WM_KEYUP:
        switch (wParam) {
        case KEY_W:
            if (movement_.forward_ > 0) {
                movement_.forward_ = 0;
            }
            break;
        case KEY_S:
            if (movement_.forward_ < 0) {
                movement_.forward_ = 0;
            }
            break;
        case KEY_A:
            if (movement_.right_ < 0) {
                movement_.right_ = 0;
            }
            break;
        case KEY_D:
            if (movement_.right_ > 0) {
                movement_.right_ = 0;
            }
            break;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK VkDemo::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    VkDemo* obj = (VkDemo*)GetWindowLongPtr(hWnd, 0);
    return obj->DemoWndProc(hWnd, msg, wParam, lParam);
}
#endif

bool VkDemo::CreateVkInstance() {
    VkApplicationInfo app_info = {};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "Demo";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Demo";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo inst_create_info = {};

    inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pNext = nullptr;
    inst_create_info.flags = 0;
    inst_create_info.pApplicationInfo = &app_info;

    std::vector<const char*> enabled_layernames;

#if defined(_DEBUG)
    enabled_layernames.push_back("VK_LAYER_KHRONOS_validation");    // VK_API_VERSION_1_1
#endif

    std::vector<const char*> instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };

#if defined(_WIN32)
    instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

#if defined(_DEBUG)
    instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    AddAdditionalInstanceExtensions(instance_extensions);

    inst_create_info.enabledLayerCount = (uint32_t)enabled_layernames.size();
    inst_create_info.ppEnabledLayerNames = enabled_layernames.data();

    inst_create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
    inst_create_info.ppEnabledExtensionNames = instance_extensions.data();

    VkResult rt = vkCreateInstance(&inst_create_info, nullptr, &vk_instance_);

    if (rt == VK_SUCCESS) {
#ifdef _DEBUG
        /* Load VK_EXT_debug_report entry points in debug builds */
        vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(vk_instance_, "vkCreateDebugReportCallbackEXT"));
        vkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(vk_instance_, "vkDebugReportMessageEXT"));
        vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(vk_instance_, "vkDestroyDebugReportCallbackEXT"));

        if (!vkCreateDebugReportCallbackEXT || !vkDebugReportMessageEXT || !vkDestroyDebugReportCallbackEXT) {
            printf("could not get vulkan validation function pointers\n");
            return false;
        }

        /* Setup callback creation information */
        VkDebugReportCallbackCreateInfoEXT callback_create_info;
        callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        callback_create_info.pNext = nullptr;
        callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        callback_create_info.pfnCallback = &DebugReportCallback;
        callback_create_info.pUserData = this;

        // Register the callback 
        if (vkCreateDebugReportCallbackEXT(vk_instance_, &callback_create_info, nullptr, &debug_report_callback_) != VK_SUCCESS) {
            printf("could not create debug report callback\n");
            debug_report_callback_ = nullptr;
            return false;
        }
#endif

        return true;
    }
    else {
        return false;
    }
}

void VkDemo::DestroyVkInstance() {
    if (vk_instance_) {
#ifdef _DEBUG
        if (vkDestroyDebugReportCallbackEXT && debug_report_callback_) {
            vkDestroyDebugReportCallbackEXT(vk_instance_, debug_report_callback_, nullptr);
            debug_report_callback_ = nullptr;
        }

        // clear function pointers
        vkCreateDebugReportCallbackEXT = nullptr;
        vkDebugReportMessageEXT = nullptr;
        vkDestroyDebugReportCallbackEXT = nullptr;
#endif

        vkDestroyInstance(vk_instance_, nullptr);
        vk_instance_ = VK_NULL_HANDLE;
    }
}

#ifdef _DEBUG
VkBool32 VkDemo::DemoDebugReportCallback(const char* pMessage) {
    printf("---------- Error ----------\n%s\n", pMessage);
    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkDemo::DebugReportCallback(VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage, void* pUserData) 
{
    return ((VkDemo*)pUserData)->DemoDebugReportCallback(pMessage);
}
#endif

// physical device
bool VkDemo::SelectPhysicalDevice() {
    uint32_t physical_device_count = 0;
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(vk_instance_, &physical_device_count, nullptr)) {
        return false;
    }

    if (!physical_device_count) {
        printf("No physical device\n");
        return false;
    }

    // choose a physical device that support VK_QUEUE_GRAPHICS_BIT

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(vk_instance_, &physical_device_count, physical_devices.data())) {
        return false;
    }

    VkPhysicalDevice hPhysicalDevice = NULL;

    for (auto& physical_device : physical_devices) {
        uint32_t queue_family_property_count = 0;
        /* void */ vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, queue_family_properties.data());

        bool support_graphics = false;

        for (uint32_t j = 0; j < queue_family_property_count; ++j) {
            VkQueueFamilyProperties& p = queue_family_properties[j];
            if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) { // that queue family support graphics
                vk_physical_device_graphics_queue_family_index_ = j;
                support_graphics = true;
                break;
            }
        }

        if (support_graphics) {
            vk_physical_device_ = physical_device;

            vkGetPhysicalDeviceFeatures(physical_device, &vk_physical_device_features_);

            vkGetPhysicalDeviceMemoryProperties(physical_device, &vk_physical_device_memory_properties_);

            // provided by version 1.1
            vk_physical_device_mesh_shader_propertices_.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
            vk_physical_device_mesh_shader_propertices_.pNext = nullptr;

            vk_physical_device_properties2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vk_physical_device_properties2_.pNext = &vk_physical_device_mesh_shader_propertices_;

            vkGetPhysicalDeviceProperties2(physical_device, &vk_physical_device_properties2_);

            printf("DeviceName = \"%s\"\n", vk_physical_device_properties2_.properties.deviceName);
            printf("nonCoherentAtomSize = %llu\n", vk_physical_device_properties2_.properties.limits.nonCoherentAtomSize);
            printf("maxTaskWorkGroupCount = [%u,%u,%u]\n", 
                vk_physical_device_mesh_shader_propertices_.maxTaskWorkGroupCount[0],
                vk_physical_device_mesh_shader_propertices_.maxTaskWorkGroupCount[1],
                vk_physical_device_mesh_shader_propertices_.maxTaskWorkGroupCount[2]);
            printf("maxMeshWorkGroupCount = [%u,%u,%u]\n", 
                vk_physical_device_mesh_shader_propertices_.maxMeshWorkGroupCount[0],
                vk_physical_device_mesh_shader_propertices_.maxMeshWorkGroupCount[1],
                vk_physical_device_mesh_shader_propertices_.maxMeshWorkGroupCount[2]);
            printf("maxMeshOutputVertices = %u\n", vk_physical_device_mesh_shader_propertices_.maxMeshOutputVertices);
            printf("maxMeshOutputPrimitives = %u\n", vk_physical_device_mesh_shader_propertices_.maxMeshOutputPrimitives);

            return true;
        }
    }

    return false; // no physical device support graphics queue
}

// init supported depth format
bool VkDemo::InitSupportedDepthFormat() {
    // front highest precision to lowest
    std::array<VkFormat, 5> depth_formats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depth_formats) {
        VkFormatProperties format_propts = {};
        vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_propts);
        if (format_propts.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            vk_depth_format_ = format;
            return true;
        }
    }

    return false;
}

// logic device
bool VkDemo::CreateDevice() {
    VkDeviceQueueCreateInfo device_queue_create_info = {};

    // The priority of each queue is a normalized floating-point value tetween 0.0 and 1.0,
    // which is then tranlated to a decreate priority level by the implementation.
    // Higher value indicate a higher priority,
    // with 0.0 being the lowest priority and 1.0 being the highest.

    float queue_priorities[1];  // only queue in here

    queue_priorities[0] = 0.0f; // lowest

    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.pNext = nullptr;
    // vkGetDeviceQueue must only be used to get queues that were crated with flags 
    // paramter of VkDeviceQueueCreateInfo set to 0
    // to get queues that were created with a non-zero flags paramter use
    // vkGetDeviceQueue2
    device_queue_create_info.flags = 0; // 
    // which queue family 
    device_queue_create_info.queueFamilyIndex = vk_physical_device_graphics_queue_family_index_;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = queue_priorities;


    VkDeviceCreateInfo device_create_info = {};

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = vk_device_create_next_chain_;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &device_queue_create_info;
    // enabledLayerCount is deprecated and should not be used
    device_create_info.enabledLayerCount = 0;
    // ppEnabledLayerNames is deprecated and should not be used
    device_create_info.ppEnabledLayerNames = nullptr;

    std::vector<const char*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // VK_KHR_Maintenance1
    device_extensions.push_back(VK_KHR_MAINTENANCE_1_EXTENSION_NAME);

    AddAdditionalDeviceExtensions(device_extensions);

    device_create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    // enable wireframe mode
    VkPhysicalDeviceFeatures enable_features = vk_physical_device_features_;
    enable_features.fillModeNonSolid = VK_TRUE;

    device_create_info.pEnabledFeatures = &enable_features;

    VkResult rt = vkCreateDevice(vk_physical_device_, &device_create_info, nullptr, &vk_device_);

    if (rt == VK_SUCCESS) {
        vkGetDeviceQueue(vk_device_, vk_physical_device_graphics_queue_family_index_, 0, &vk_graphics_queue_);
        return true;
    }
    else {
        return false;
    }
}

void VkDemo::DestroyDevice() {
    vk_graphics_queue_ = VK_NULL_HANDLE;

    if (vk_device_) {
        vkDestroyDevice(vk_device_, nullptr);
        vk_device_ = VK_NULL_HANDLE;
    }
}

// semaphores used in this demo
bool VkDemo::CreateDemoSemaphores() {
    auto CreateSamaphore = [&](VkSemaphore& sem) -> bool {

        VkSemaphoreCreateInfo semaphore_create_info = {};

        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;

        return VK_SUCCESS == vkCreateSemaphore(vk_device_, &semaphore_create_info, nullptr, &sem);
    };

    return CreateSamaphore(vk_semaphore_present_complete_) && CreateSamaphore(vk_semaphore_render_complete_);
}

void VkDemo::DestroyDemoSemaphores() {
    auto DestroySamaphore = [&](VkSemaphore& sem) {
        
        if (sem) {
            vkDestroySemaphore(vk_device_, sem, nullptr);
            sem = VK_NULL_HANDLE;
        }
        
    };

    DestroySamaphore(vk_semaphore_render_complete_);
    DestroySamaphore(vk_semaphore_present_complete_);
}

// surface
bool VkDemo::CreateSurface() {
#if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;  // must be 0
    create_info.hinstance = hInstance_;
    create_info.hwnd = hWnd_;

    VkResult rt = vkCreateWin32SurfaceKHR(vk_instance_, &create_info,
        nullptr, &vk_surface_);

    if (rt == VK_SUCCESS) {

        // check support present in a platform-neutral manner
        VkBool32 support_presentation = VK_FALSE;

        vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device_,
            vk_physical_device_graphics_queue_family_index_,
            vk_surface_, &support_presentation);

        if (!support_presentation) {
            vkDestroySurfaceKHR(vk_instance_, vk_surface_, nullptr);
            vk_surface_ = VK_NULL_HANDLE;
            printf("surface not support presentation\n");
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
#else
    return false;   // TODO: other platform
#endif
}

void VkDemo::DestroySurface() {
    if (vk_surface_) {
        vkDestroySurfaceKHR(vk_instance_, vk_surface_, nullptr);
        vk_surface_ = VK_NULL_HANDLE;
    }
}

// swapchain
bool VkDemo::CreateSwapChain(bool vsync) {
    // -- min image count --

    // get phsyical device surface propertices and formats
    VkSurfaceCapabilitiesKHR surf_caps = {};
    VkResult rt = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
        &surf_caps);
    if (rt != VK_SUCCESS) {
        return false;
    }

    // image count
    uint32_t desired_swapchain_image_count = surf_caps.minImageCount + 1;
    if (surf_caps.maxImageCount && desired_swapchain_image_count > surf_caps.maxImageCount) {
        // clamp
        desired_swapchain_image_count = surf_caps.maxImageCount;
    }

    // -- select image format & color space
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkColorSpaceKHR image_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    uint32_t surface_format_count = 0;
    rt = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_,
        &surface_format_count, nullptr);
    if (rt != VK_SUCCESS) {
        return false;
    }

    if (surface_format_count == 0) {
        printf("no surface format\n");
        return false;
    }

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    rt = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_,
        &surface_format_count, surface_formats.data());
    if (rt != VK_SUCCESS) {
        return false;
    }

    if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        image_format = VK_FORMAT_R8G8B8A8_UNORM;
        image_color_space = surface_formats[0].colorSpace;
    }
    else {
        bool found_FORMAT_R8G8B8A8_UNORM = false;
        for (uint32_t i = 0; i < surface_format_count; ++i) {
            if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
                found_FORMAT_R8G8B8A8_UNORM = true;
                image_color_space = surface_formats[i].colorSpace;
                break;
            }
        }

        if (!found_FORMAT_R8G8B8A8_UNORM) {
            // select first item
            image_format = surface_formats[0].format;
            image_color_space = surface_formats[0].colorSpace;
        }
    }

    // -- image_extent --
    VkExtent2D image_extent = {};

    if (surf_caps.currentExtent.width == (uint32_t)-1) {
        // special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that the surface size
        // will be determined by the extent of a swapchain targeting the surface.

        image_extent.width = cfg_viewport_cx_;
        image_extent.height = cfg_viewport_cy_;
    }
    else {
        image_extent = surf_caps.currentExtent;

        cfg_viewport_cx_ = image_extent.width;
        cfg_viewport_cy_ = image_extent.height;
    }

    // -- image_usage --
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Set additional usage flag for blitting from the swapchain images if supported. 
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(vk_physical_device_, image_format, &format_props);   // return void
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // -- pre_transform --

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        //    specifies that image content is presented without being trnasformed.
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        pre_transform = surf_caps.currentTransform;
    }

    // -- composite_alpha --

    // Find a supported composite alphe format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    std::array< VkCompositeAlphaFlagBitsKHR, 4> composite_alpha_flag_bits = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    for (auto& it : composite_alpha_flag_bits) {
        if (surf_caps.supportedCompositeAlpha & it) {
            composite_alpha = it;
            break;
        }
    }

    // -- present mode --
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync) {
        // if v-sync is disabled we try to find a mailbox mode.
        // It's the lowest latency non-tearing present mode available.

        uint32_t present_mode_count = 0;
        VkResult local_rt = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_,
            vk_surface_, &present_mode_count, nullptr);

        if (local_rt == VK_SUCCESS && present_mode_count) {
            std::vector<VkPresentModeKHR> present_modes(present_mode_count);

            if (vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_,
                vk_surface_, &present_mode_count, present_modes.data()) == VK_SUCCESS) {

                for (auto& check_present_mode : present_modes) {
                    if (check_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        present_mode = VK_PRESENT_MODE_MAILBOX_KHR;  // support VK_PRESENT_MODE_MAILBOX_KHR
                        break;
                    }
                }

                if (present_mode != VK_PRESENT_MODE_MAILBOX_KHR) {
                    // try VK_PRESENT_MODE_IMMEDIATE_KHR mode
                    for (auto& check_present_mode : present_modes) {
                        if (check_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                            break;
                        }
                    }
                }
            }
        }
    }

    // fill create info

    VkSwapchainCreateInfoKHR create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.surface = vk_surface_;
    create_info.minImageCount = desired_swapchain_image_count;
    create_info.imageFormat = image_format;
    create_info.imageColorSpace = image_color_space;
    create_info.imageExtent = image_extent;

    // imageArrayLayers: is the number of views in a multiview/stereo surface.
    //                   For non-stereoscopic 3D applications this value is 1
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = image_usage;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // no queue family indices, because the imageSharingMode is not VK_SHARING_MODE_CONCURRENT
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = nullptr;
    create_info.preTransform = pre_transform;
    create_info.compositeAlpha = composite_alpha;
    create_info.presentMode = present_mode;

    // setting clipped to VK_TRUE allows the implementation to discard rendering outsize of the surface area.
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = vk_swapchain_;

    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
    rt = vkCreateSwapchainKHR(vk_device_, &create_info, nullptr, &new_swapchain);
    vk_swapchain_color_format_ = image_format;

    DestroySwapChain();
    vk_swapchain_ = new_swapchain;

    if (rt == VK_SUCCESS) {
        uint32_t swapchain_image_count = 0;

        if (vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &swapchain_image_count, nullptr) == VK_SUCCESS) {
            std::vector<VkImage> swapchain_images(swapchain_image_count);

            vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &swapchain_image_count, swapchain_images.data());
            if (swapchain_image_count > MAX_SWAPCHAIN_IMAGES) {
                printf("swapchain_image_count > MAX_SWAPCHAIN_IMAGES (%d)\n", MAX_SWAPCHAIN_IMAGES);
                return false;
            }

            for (uint32_t i = 0; i < swapchain_image_count; ++i) {
                vk_swapchain_images_[i].image_ = swapchain_images[i];

                VkImageViewCreateInfo image_view_create_info = {};

                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                image_view_create_info.pNext = nullptr;
                image_view_create_info.flags = 0;
                image_view_create_info.image = swapchain_images[i];
                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                image_view_create_info.format = image_format;
                image_view_create_info.components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A
                };

                // selecting the set of mipmap levels and array layers to be accessable to the view
                image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // which aspect(s) of the image are included in the view
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = 1;
                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                image_view_create_info.subresourceRange.layerCount = 1;

                if (vkCreateImageView(vk_device_, &image_view_create_info, nullptr,
                    &vk_swapchain_images_[i].image_view_) != VK_SUCCESS) {
                    return false;
                }

                vk_swapchain_image_count_++;
            }

            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

void VkDemo::DestroySwapChain() {
    if (vk_swapchain_) {

        for (int i = 0; i < vk_swapchain_image_count_; ++i) {
            if (vk_swapchain_images_[i].image_view_) {
                vkDestroyImageView(vk_device_, vk_swapchain_images_[i].image_view_, nullptr);
                vk_swapchain_images_[i].image_view_ = VK_NULL_HANDLE;
            }
        }
        vk_swapchain_image_count_ = 0;

        vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);
        vk_swapchain_ = VK_NULL_HANDLE;
    }
}

// fences used in thie demo
bool VkDemo::CreateDemoFences() {
    vk_wait_fence_count_ = 0;

    for (int i = 0; i < vk_swapchain_image_count_; ++i) {
        VkFenceCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // create signed

        if (VK_SUCCESS != vkCreateFence(vk_device_, &create_info, nullptr, vk_wait_fences_ + i)) {
            return false;
        }

        vk_wait_fence_count_++;
    }

    return true;
}

void VkDemo::DestroyDemoFences() {
    for (int i = 0; i < vk_wait_fence_count_; ++i) {
        if (vk_wait_fences_[i]) {
            vkDestroyFence(vk_device_, vk_wait_fences_[i], nullptr);
            vk_wait_fences_[i] = VK_NULL_HANDLE;
        }
    }
    vk_wait_fence_count_ = 0;
}

// depth stencil
bool VkDemo::CreateDepthStencil() {
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_depth_format_,
        .extent = { cfg_viewport_cx_, cfg_viewport_cy_, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (VK_SUCCESS != vkCreateImage(vk_device_, &image_create_info, nullptr, &vk_depth_stencil_.image_)) {
        return false;
    }

    /*
    typedef struct VkMemoryRequirements {
        VkDeviceSize    size;
        VkDeviceSize    alignment;
        uint32_t        memoryTypeBits;
    } VkMemoryRequirements;

    memoryTypeBits: is a bitmask and contains one bit set for every supported memory type for the
        resource. Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties
        structure for the physical device is supported for the resource.
    */

    VkMemoryRequirements image_memory_requirements = {};
    vkGetImageMemoryRequirements(vk_device_, vk_depth_stencil_.image_, &image_memory_requirements); // return void

    uint32_t memory_type_index = GetMemoryTypeIndex(image_memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (memory_type_index == (uint32_t)-1) {
        return false;
    }

    VkMemoryAllocateInfo mem_alloc_info = {};

    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.pNext = nullptr;
    mem_alloc_info.allocationSize = image_memory_requirements.size;
    mem_alloc_info.memoryTypeIndex = memory_type_index;

    if (VK_SUCCESS != vkAllocateMemory(vk_device_, &mem_alloc_info, nullptr, &vk_depth_stencil_.memory_)) {
        return false;
    }

    // Bind device memory to an image object
    if (VK_SUCCESS != vkBindImageMemory(vk_device_, vk_depth_stencil_.image_, vk_depth_stencil_.memory_, 0)) {
        return false;
    }

    VkImageViewCreateInfo image_view_create_info = {};

    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = vk_depth_stencil_.image_;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = image_create_info.format;
    // image_view_create_info.components
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    if (VK_SUCCESS != vkCreateImageView(vk_device_,
        &image_view_create_info, nullptr, &vk_depth_stencil_.image_view_)) {
        return false;
    }

    return true;
}

void VkDemo::DestroyDepthStencil() {
    if (vk_depth_stencil_.image_view_) {
        vkDestroyImageView(vk_device_, vk_depth_stencil_.image_view_, nullptr);
        vk_depth_stencil_.image_view_ = VK_NULL_HANDLE;
    }

    if (vk_depth_stencil_.memory_) {
        vkFreeMemory(vk_device_, vk_depth_stencil_.memory_, nullptr);
        vk_depth_stencil_.memory_ = VK_NULL_HANDLE;
    }

    if (vk_depth_stencil_.image_) {
        vkDestroyImage(vk_device_, vk_depth_stencil_.image_, nullptr);
        vk_depth_stencil_.image_ = VK_NULL_HANDLE;
    }
}

// render pass
//   specifiy framebuffer by command vkCmdBeginRenderPass
bool VkDemo::CreateRenderPass() {
    // -- subpass --

    VkSubpassDescription subpass_desciption = {};

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,    // first attachment is color image view
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 1,    // second attachment is depth image view
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    subpass_desciption.flags = 0;
    subpass_desciption.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desciption.inputAttachmentCount = 0;
    subpass_desciption.pInputAttachments = nullptr;
    subpass_desciption.colorAttachmentCount = 1;
    subpass_desciption.pColorAttachments = &color_attachment_reference;
    subpass_desciption.pResolveAttachments = nullptr;
    subpass_desciption.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_desciption.preserveAttachmentCount = 0;
    subpass_desciption.pResolveAttachments = nullptr;

    // -- render pass --

    VkRenderPassCreateInfo create_info = {};

    std::array<VkAttachmentDescription, 2> attachments = {};

    // color
    attachments[0].flags = 0;
    // Use the color format selected by the swapchain
    attachments[0].format = vk_swapchain_color_format_;
    // We don't use multisampling in this example
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    // Clear this attachment at the start of the render pass
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // Keep it's contents after the render pass is finished (for displaying it)
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // We don't use stencil
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // We don't use stencil
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // We want to present the color buffer to the swapchain
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // depth stencil
    attachments[1].flags = 0;
    attachments[1].format = vk_depth_format_;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // We don't need depth buffer after render pass has finished.
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // No stencil
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // No stencil
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Transition to depth/stencil attachment
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDependency, 2> dependencies = {};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.attachmentCount = 2;
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_desciption;
    create_info.dependencyCount = 2;
    create_info.pDependencies = dependencies.data();

    if (VK_SUCCESS != vkCreateRenderPass(vk_device_, &create_info, nullptr, &vk_render_pass_)) {
        return false;
    }

    return true;
}

void VkDemo::DestroyRenderPass() {
    if (vk_render_pass_) {
        vkDestroyRenderPass(vk_device_, vk_render_pass_, nullptr);
        vk_render_pass_ = VK_NULL_HANDLE;
    }
}

// framebuffers
bool VkDemo::CreateFramebuffers() {
    vk_framebuffer_count_ = 0;

    for (size_t i = 0; i < vk_swapchain_image_count_; ++i) {

        VkImageView attachments[2] = {
            vk_swapchain_images_[i].image_view_,
            vk_depth_stencil_.image_view_
        };

        VkFramebufferCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.renderPass = vk_render_pass_;
        create_info.attachmentCount = 2;
        create_info.pAttachments = attachments;
        create_info.width = cfg_viewport_cx_;
        create_info.height = cfg_viewport_cy_;
        create_info.layers = 1;

        if (VK_SUCCESS != vkCreateFramebuffer(vk_device_, &create_info,
            nullptr, vk_framebuffers_ + i)) {
            return false;
        }

        vk_framebuffer_count_++;
    }

    return true;
}

void VkDemo::DestroyFramebuffers() {
    for (int i = 0; i < vk_framebuffer_count_; ++i) {
        if (vk_framebuffers_[i]) {
            vkDestroyFramebuffer(vk_device_, vk_framebuffers_[i], nullptr);
            vk_framebuffers_[i] = VK_NULL_HANDLE;
        }
    }
    vk_framebuffer_count_ = 0;
}

// command pool
bool VkDemo::CreateCommandPool() {
    VkCommandPoolCreateInfo command_pool_create_info = {};

    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    // allow reset individuel command buffer var vkResetCommandBuffer
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // command buffers from this pool can only be submitted no queues 
    //   corresponding to this queue family.
    command_pool_create_info.queueFamilyIndex = vk_physical_device_graphics_queue_family_index_;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkResult rt = vkCreateCommandPool(vk_device_, &command_pool_create_info, nullptr, &command_pool);

    if (rt == VK_SUCCESS) {
        vk_command_pool_ = command_pool;
        return true;
    }
    else {
        return false;
    }
}

void VkDemo::DestroyCommandPool() {
    if (vk_command_pool_) {
        vkDestroyCommandPool(vk_device_, vk_command_pool_, nullptr);
        vk_command_pool_ = VK_NULL_HANDLE;
    }
}

// command buffers
bool VkDemo::AllocCommandBuffers() {
    VkCommandBufferAllocateInfo alloc_info = {};

    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = vk_command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t)vk_swapchain_image_count_;

    memset(vk_draw_cmd_buffers_, 0, sizeof(vk_draw_cmd_buffers_));

    if (VK_SUCCESS == vkAllocateCommandBuffers(vk_device_, &alloc_info, vk_draw_cmd_buffers_)) {
        vk_draw_cmd_buffer_count_ = vk_swapchain_image_count_;
        return true;
    }
    else {
        return false;
    }
}

void VkDemo::FreeCommandBuffers() {
    if (vk_draw_cmd_buffers_[0]) {
        vkFreeCommandBuffers(vk_device_, vk_command_pool_,
            (uint32_t)vk_draw_cmd_buffer_count_, vk_draw_cmd_buffers_);
    }
    vk_draw_cmd_buffer_count_ = 0;
}

// pipeline
bool VkDemo::CreatePipelineCache() {
    VkPipelineCacheCreateInfo create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.initialDataSize = 0;
    create_info.pInitialData = nullptr;

    return VK_SUCCESS == vkCreatePipelineCache(vk_device_, &create_info, nullptr, &vk_pipeline_cache_);
}

void VkDemo::DestroyPipelineCache() {
    if (vk_pipeline_cache_) {
        vkDestroyPipelineCache(vk_device_, vk_pipeline_cache_, nullptr);
        vk_pipeline_cache_ = VK_NULL_HANDLE;
    }
}

void VkDemo::CheckMovement() {
    if (movement_.forward_ != 0) {
        glm::vec3 forward = glm::normalize(camera_.target_ - camera_.pos_);
        glm::vec3 forward_delta = forward * (move_speed_ * movement_.forward_);
        camera_.pos_ += forward_delta;
        camera_.target_ += forward_delta;
    }

    if (movement_.right_ != 0) {
        glm::vec3 forward = glm::normalize(camera_.target_ - camera_.pos_);
        glm::vec3 right = glm::cross(forward, camera_.up_);
        glm::vec3 right_delta = right * (move_speed_ * movement_.right_);
        camera_.pos_ += right_delta;
        camera_.target_ += right_delta;
    }

}

void VkDemo::OnResize() {
    enable_display_ = false;

    if (!vk_instance_) {
        return;
    }

    DestroyFramebuffers();
    DestroyDepthStencil();
    DestroySwapChain();
    DestroySurface();

    if (!CreateSurface()) {
        return;
    }

    if (!CreateSwapChain(false)) {
        return;
    }

    if (!CreateDepthStencil()) {
        return;
    }

    if (!CreateFramebuffers()) {
        return;
    }

    // rebuld command buffer
    BuildCommandBuffers();

    enable_display_ = true;
}


