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
helper
================================================================================
*/
// image_memory_barrier: in/out
static void SetAccessMaskOfImageMemoryBarrier(VkImageMemoryBarrier & image_memory_barrier) {
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    
    switch (image_memory_barrier.oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        image_memory_barrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        printf("Not processed old image layout %d\n", (int)image_memory_barrier.oldLayout);
        break;
    }
    
    switch (image_memory_barrier.newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (image_memory_barrier.srcAccessMask == 0) {
            image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        printf("Not processed new image layout %d\n", (int)image_memory_barrier.newLayout);
        break;
    }
}

/*
================================================================================
VkDemo
================================================================================
*/
VkDemo::VkDemo():
    camera_mode_(camera_mode_t::CM_MOVE_CAMERA),
    camera_rotation_flags_(ROTATION_YAW_BIT | ROTATION_PITCH_BIT),
    z_near_(1.0f),
    z_far_(16.0f),
    fovy_(70.0f),
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
    vk_command_pool_transient_(VK_NULL_HANDLE),
    enable_display_(false),
    model_scale_(1.0f),
    move_speed_(2.0f),
    model_rotate_mat_(nullptr),
    left_button_down_(false),
    cursor_x_(0),
    cursor_y_(0),
    mouse_sensitive_(0.5f)
{
	shaders_dir_[0] = '\0';
    textures_dir_[0] = '\0';
    models_dir_[0] = '\0';

    model_rotate_mat_ = new glm::mat4(1.0f);

	memset(&vk_physical_device_memory_properties_, 0, sizeof(vk_physical_device_memory_properties_));
	memset(&vk_physical_device_properties2_, 0, sizeof(vk_physical_device_properties2_));
	memset(&vk_physical_device_mesh_shader_propertices_, 0, sizeof(vk_physical_device_mesh_shader_propertices_));
	memset(&vk_physical_device_features_, 0, sizeof(vk_physical_device_features_));

    memset(&vk_swapchain_images_, 0, sizeof(vk_swapchain_images_));
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

    memset(vk_framebuffers_, 0, sizeof(vk_framebuffers_));
    memset(vk_draw_cmd_buffers_, 0, sizeof(vk_draw_cmd_buffers_));

    memset(vk_wait_fences_, 0, sizeof(vk_wait_fences_));

#if defined(_WIN32)
    hInstance_ = GetModuleHandle(NULL);
#endif


    /*
    
    // all the layers
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> props;
    props.resize(count);

    vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

    printf("---- extensions begin ----\n");
    for (auto& it : props) {
        printf("%s\n", it.extensionName);
    }
    printf("---- extensions end ----\n");

    */
}

VkDemo::~VkDemo() {
    delete model_rotate_mat_;
}

bool VkDemo::Init(const char* project_shader_dir,
    uint32_t max_uniform_buffer,
    uint32_t max_storage_buffer,
    uint32_t max_texture,
    uint32_t max_desp_set) 
{
    // setup folders
    Str_SPrintf(shaders_dir_, MAX_PATH, "%s/%s",
        GetShadersFolder(), project_shader_dir);
    
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

    vk_depth_format_ = GetIdealDepthStencilFormat();
    if (vk_depth_format_ == VK_FORMAT_UNDEFINED) {
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

    if (!CreateCommandPools()) {
        return false;
    }

    if (!AllocCommandBuffers()) {
        return false;
    }

    if (!CreatePipelineCache()) {
        return false;
    }

    if (!CreateDescriptorPools(max_uniform_buffer, max_storage_buffer,
        max_texture, max_desp_set)) {
        return false;
    }

    return true;
}

void VkDemo::Shutdown() {
    DestroyDescriptorPools();
    DestroyPipelineCache();
    FreeCommandBuffers();
    DestroyCommandPools();
    DestroyFramebuffers();
    DestroyRenderPass(vk_render_pass_);
    DestroyDepthStencil();
    DestroyDemoFences();
    DestroySwapChain();
    DestroySurface();
    DestroyDemoSemaphores();
    DestroyDevice();
    DestroyVkInstance();
}

void VkDemo::Display() {
    uint32_t current_image_idx = 0;
    VkResult rt;

    Update();

    // If semaphore is not VK_NULL_HANDLE, it must not have any uncompleted signal or wait
    //   operations pending
    // If fence is not VK_NULL_HANDLE, fence must be unsignaled
    // semaphore and fence must not both be equal to VK_NULL_HANDLE
    rt = vkAcquireNextImageKHR(vk_device_, vk_swapchain_, UINT64_MAX /* never timeout */,
        vk_semaphore_present_complete_, VK_NULL_HANDLE, &current_image_idx);

    if (rt != VK_SUCCESS) {
        printf("vkAcquireNextImageKHR error\n");
        return;
    }

    VkFence fence = vk_wait_fences_[current_image_idx];

    vkWaitForFences(vk_device_, 1, &fence, VK_TRUE /* waitAll */, UINT64_MAX /* never timeout */);
    vkResetFences(vk_device_, 1, &fence);	// set the state of current fence to unsignal.

    VkSubmitInfo submit_info = {};

    VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vk_semaphore_present_complete_;
    submit_info.pWaitDstStageMask = &pipeline_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_draw_cmd_buffers_[current_image_idx];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &vk_semaphore_render_complete_;

    rt = vkQueueSubmit(vk_graphics_queue_, 1, &submit_info, fence);
    if (rt != VK_SUCCESS) {
        printf("vkQueueSubmit error\n");
        return;
    }

    VkPresentInfoKHR present_info = {};

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &vk_semaphore_render_complete_;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vk_swapchain_;
    present_info.pImageIndices = &current_image_idx;
    present_info.pResults = nullptr;

    rt = vkQueuePresentKHR(vk_graphics_queue_, &present_info);
    if (rt != VK_SUCCESS) {
        printf("vkQueuePresentKHR error\n");
        return;
    }

    rt = vkQueueWaitIdle(vk_graphics_queue_);
    if (rt != VK_SUCCESS) {
        printf("vkQueueWaitIdle error\n");
        return;
    }
}

void VkDemo::WindowSizeChanged() {
    // override by child class
}

void VkDemo::Update() {
    // override by child class
}

void VkDemo::MainLoop() {
#if defined(_WIN32)

    if (enable_display_) {
        Display();
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

// interface
VkDevice VkDemo::GetDevice() const {
    return vk_device_;
}

VkDescriptorPool VkDemo::GetDescriptorPool() const {
    return vk_descriptor_pool_;
}

size_t VkDemo::GetAlignedBufferSize(size_t sz) const {
    // The Vulkan spec states: If size is not equal to VK_WHOLE_SIZE, size must either be a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize, 
        // or offset plus size must equal the size of memory
    VkDeviceSize nonCoherentAtomSize = vk_physical_device_properties2_.properties.limits.nonCoherentAtomSize;

    if (sz % nonCoherentAtomSize) {
        return ((sz / nonCoherentAtomSize) + 1) * nonCoherentAtomSize;
    }
    else {
        return sz;
    }
}

size_t VkDemo::GetAlignedMinOffsetSize(size_t sz) const {
    size_t limit = vk_physical_device_properties2_.properties.limits.minUniformBufferOffsetAlignment;
    if (sz % limit) {
        return ((sz / limit) + 1) * limit;
    }
    else {
        return sz;
    }
}

bool VkDemo::LoadModel(const char* filename, bool move_to_origin, 
    model_s& model, const glm::mat4* transform) const
{
    char full_filename[MAX_PATH];
    Str_SPrintf(full_filename, MAX_PATH, "%s/%s", models_dir_, filename);

    return Model_Load(full_filename, move_to_origin, model, transform);
}

bool VkDemo::CreateBuffer(vk_buffer_s& buffer, 
    VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_prop_flags, VkDeviceSize req_size) const
{
    VkDeviceSize aligned_req_size = GetAlignedBufferSize(req_size);

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
        .memoryTypeIndex = GetMemoryTypeIndex(mem_req.memoryTypeBits, mem_prop_flags)
    };

    if (VK_SUCCESS != vkAllocateMemory(vk_device_, &mem_alloc_info, nullptr, &buffer.memory_)) {
        return false;
    }

    buffer.memory_prop_flags_ = mem_prop_flags;
    buffer.memory_size_ = aligned_req_size; // not mem_req.size;

    if (VK_SUCCESS != vkBindBufferMemory(vk_device_, buffer.buffer_, buffer.memory_, 0)) {
        return false;
    }

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

bool VkDemo::UpdateBuffer(vk_buffer_s& buffer, const void* host_data, size_t host_data_size) const {
    void* data = MapBuffer(buffer);
    if (!data) {
        return false;
    }

    memcpy(data, host_data, host_data_size);

    return UnmapBuffer(buffer);
}

void* VkDemo::MapBuffer(vk_buffer_s& buffer) const {
    // entire range: TO FIX
    VkDeviceSize offset = 0;
    VkDeviceSize range = buffer.memory_size_;

    void* data = nullptr;
    VkResult rt = vkMapMemory(vk_device_, buffer.memory_, offset, range, 0, &data);
    if (rt != VK_SUCCESS) {
        printf("[MapBuffer] vkMapMemory error\n");
        return nullptr;
    }

    return data;
}

bool VkDemo::UnmapBuffer(vk_buffer_s& buffer) const {
    bool ok = true;

    if ((buffer.memory_prop_flags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        // not use host cache management commands

        // flush to make change visible to device
        VkMappedMemoryRange mapped_range = {};

        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.pNext = nullptr;
        mapped_range.memory = buffer.memory_;
        mapped_range.offset = 0;
        mapped_range.size = buffer.memory_size_;

        VkResult rt = vkFlushMappedMemoryRanges(vk_device_, 1, &mapped_range);
        if (rt != VK_SUCCESS) {
            ok = false;
            printf("[UpdateBuffer] vkFlushMappedMemoryRanges error\n");
        }
    }

    vkUnmapMemory(vk_device_, buffer.memory_);

    return ok;
}

// Staging buffer
// https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

bool VkDemo::CreateBufferAddInitData(vk_buffer_s& buffer, VkBufferUsageFlags usage_flags, 
    const void* data, size_t data_size, bool staging) 
{
    vk_buffer_s temp_buffer = {};

    VkBufferUsageFlags temp_buffer_usage_flags = usage_flags;
    if (staging) {
        temp_buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (!CreateBuffer(temp_buffer, temp_buffer_usage_flags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        data_size)) {
        DestroyBuffer(temp_buffer);
        return false;
    }

    if (!UpdateBuffer(temp_buffer, data, data_size)) {
        return false;
    }

    if (!staging) {
        buffer = temp_buffer;
        return true;
    }

    if (!CreateBuffer(buffer, usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, data_size)) {
        DestroyBuffer(buffer);
        DestroyBuffer(temp_buffer);
        return false;
    }

    VkCommandPool command_pool = vk_command_pool_transient_;

    VkCommandBuffer cmd_buffer_copy_buffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};

    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.pNext = nullptr;
    cmd_buffer_alloc_info.commandPool = command_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = 1;

    if (VK_SUCCESS != vkAllocateCommandBuffers(vk_device_, &cmd_buffer_alloc_info, &cmd_buffer_copy_buffer)) {
        DestroyBuffer(buffer);
        DestroyBuffer(temp_buffer);
        return false;
    }

    VkCommandBufferBeginInfo cmdbuf_begin_info = {};

    cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbuf_begin_info.pNext = nullptr;
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbuf_begin_info.pInheritanceInfo = nullptr;

    if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buffer_copy_buffer, &cmdbuf_begin_info)) {
        vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_copy_buffer);
        DestroyBuffer(buffer);
        DestroyBuffer(temp_buffer);
        return false;
    }

    VkBufferCopy buffer_copy = {};

    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = data_size;

    vkCmdCopyBuffer(cmd_buffer_copy_buffer, temp_buffer.buffer_, buffer.buffer_, 1, &buffer_copy);

    if (VK_SUCCESS != vkEndCommandBuffer(cmd_buffer_copy_buffer)) {
        vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_copy_buffer);
        DestroyBuffer(buffer);
        DestroyBuffer(temp_buffer);
        return false;
    }

    SubmitCommandBufferAndWait(cmd_buffer_copy_buffer);

    vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_copy_buffer);

    // free staging buffer
    DestroyBuffer(temp_buffer);

    return true;
}

bool VkDemo::CreateVertexBuffer(vk_buffer_s& buffer, const void* data, size_t data_size, bool staging) {
    if (data) {
        return CreateBufferAddInitData(buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data, data_size, staging);
    }
    else {
        return CreateBuffer(buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data_size);
    }
}

bool VkDemo::CreateIndexBuffer(vk_buffer_s& buffer, const void* data, size_t data_size, bool staging) {
    if (data) {
        return CreateBufferAddInitData(buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data, data_size, staging);
    }
    else {
        return CreateBuffer(buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data_size);
    }
}

bool VkDemo::CreateSampler(VkFilter mag_filter, VkFilter min_filter, VkSamplerMipmapMode mipmap_mode,
    VkSamplerAddressMode address_mode, VkSampler& sampler)
{
    VkSamplerCreateInfo create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.magFilter = mag_filter;
    create_info.minFilter = min_filter;
    create_info.mipmapMode = mipmap_mode;
    create_info.addressModeU = address_mode;
    create_info.addressModeV = address_mode;
    create_info.addressModeW = address_mode;
    create_info.mipLodBias = 0.0f;
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0f;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_NEVER;
    create_info.minLod = 0.0f;
    create_info.maxLod = 1.0f;
    create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // if clamp to border
    create_info.unnormalizedCoordinates = VK_FALSE;

    return VK_SUCCESS == vkCreateSampler(vk_device_, &create_info, nullptr, &sampler);
}

void VkDemo::DestroySampler(VkSampler& sampler) {
    if (sampler) {
        vkDestroySampler(vk_device_, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
}

bool VkDemo::Create2DImage(vk_image_s& vk_image, VkImageCreateFlags flags, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t array_layers,
    VkMemoryPropertyFlags memory_property_flags,
    VkImageAspectFlags image_aspect_flags,
    VkImageViewType image_view_type,
    VkSampler sampler,   
    VkImageLayout image_layout)
{
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1},
        .mipLevels = 1,
        .arrayLayers = array_layers,
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

    vk_image.memory_prop_flags_ = memory_property_flags;

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
    image_view_create_info.viewType = image_view_type;
    image_view_create_info.format = format;
    // components
    image_view_create_info.subresourceRange.aspectMask = image_aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = array_layers;

    if (VK_SUCCESS != vkCreateImageView(vk_device_, &image_view_create_info,
        nullptr, &vk_image.image_view_)) {
        return false;
    }

    vk_image.width_ = width;
    vk_image.height_ = height;

    vk_image.desc_image_info_.sampler = sampler;
    vk_image.desc_image_info_.imageView = vk_image.image_view_;
    vk_image.desc_image_info_.imageLayout = image_layout;

    return true;
}

void VkDemo::DestroyImage(vk_image_s& vk_image) {
    if (vk_image.image_view_) {
        vkDestroyImageView(vk_device_, vk_image.image_view_, nullptr);
    }

    if (vk_image.memory_) {
        vkFreeMemory(vk_device_, vk_image.memory_, nullptr);
    }

    if (vk_image.image_) {
        vkDestroyImage(vk_device_, vk_image.image_, nullptr);
    }

    memset(&vk_image, 0, sizeof(vk_image));
}

bool VkDemo::Load2DTexture(const char* filename,
    VkFormat format, VkImageUsageFlags image_usage,
    VkSampler sampler, VkImageLayout image_layout, vk_image_s& vk_image) 
{
    memset(&vk_image, 0, sizeof(vk_image));
    
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_properties);

    if (!(format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        printf("linearTilingFeatures not support VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT\n");
        return false;
    }

    char full_filename[MAX_PATH];

    if (filename[0] == '/' || filename[1] == ':') {
        Str_Copy(full_filename, MAX_PATH, filename);    // absolute push
    }
    else {
        // relative path, load from textures folder
        Str_SPrintf(full_filename, MAX_PATH, "%s/%s", textures_dir_, filename);
    }

    image_s pic = {};
    if (!Img_Load(full_filename, pic)) {
        printf("Failed to load texture \"%s\"\n", full_filename);
        return false;
    }

    // VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation-dependent arrangement, 
    //                          for more efficient memory access

    // VK_IMAGE_TILING_LINEAR: specifies linear tiling (texels are laid out in memory in row-major order, 
    //                         possibly with some padding on each row).
    if (!Create2DImage(vk_image, 0, format, VK_IMAGE_TILING_LINEAR, image_usage,
        (uint32_t)pic.width_, (uint32_t)pic.height_, 1,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, sampler, image_layout))
    {
        Img_Free(pic);
        return false;
    }

    vk_image.width_ = (uint32_t)pic.width_;
    vk_image.height_ = (uint32_t)pic.height_;

    bool ok = Update2DTexture(pic, vk_image);

    Img_Free(pic);

    return ok;
}

bool VkDemo::Update2DTexture(const image_s& pic, vk_image_s& vk_image) {
    if (vk_image.width_ != (uint32_t)pic.width_ || vk_image.height_ != (uint32_t)pic.height_) {
        printf("image size mismatch\n");
        return false;
    }

    if (pic.format_ != image_format_t::R8G8B8A8) {
        printf("Not a RGBA pic\n");
        return false;
    }

    // copy contents
    void* mapped = nullptr;
    if (VK_SUCCESS != vkMapMemory(vk_device_, vk_image.memory_, 0, vk_image.memory_size_, 0 /* flags */, &mapped)) {
        return false;
    }

    memcpy(mapped, pic.pixels_, pic.width_ * pic.height_ * 4);

    if ((vk_image.memory_prop_flags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        // not use host cache management commands

        // flush to make change visible to device
        VkMappedMemoryRange mapped_range = {};

        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.pNext = nullptr;
        mapped_range.memory = vk_image.memory_;
        mapped_range.offset = 0;
        mapped_range.size = vk_image.memory_size_;

        if (VK_SUCCESS != vkFlushMappedMemoryRanges(vk_device_, 1, &mapped_range)) {
            vkUnmapMemory(vk_device_, vk_image.memory_);
            printf("[UpdateBuffer] vkFlushMappedMemoryRanges error\n");
            return false;
        }
    }

    vkUnmapMemory(vk_device_, vk_image.memory_);

    // change layout from VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkCommandPool command_pool = vk_command_pool_transient_;

    VkCommandBuffer cmd_buffer_load_tex = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};

    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.pNext = nullptr;
    cmd_buffer_alloc_info.commandPool = command_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = 1;

    if (VK_SUCCESS != vkAllocateCommandBuffers(vk_device_, &cmd_buffer_alloc_info, &cmd_buffer_load_tex)) {
        return false;
    }

    VkCommandBufferBeginInfo cmdbuf_begin_info = {};

    cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbuf_begin_info.pNext = nullptr;
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbuf_begin_info.pInheritanceInfo = nullptr;

    if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buffer_load_tex, &cmdbuf_begin_info)) {
        vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);
        return false;
    }

    VkImageMemoryBarrier image_memory_barrier = {};

    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
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
    SetAccessMaskOfImageMemoryBarrier(image_memory_barrier);

    vkCmdPipelineBarrier(cmd_buffer_load_tex,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0 /* dependencyFlags */,
        0 /* memoryBarrierCount */, nullptr /* pMemoryBarriers */,
        0 /* bufferMemoryBarrierCount */, nullptr /* pBufferMemoryBarries */,
        1, &image_memory_barrier);

    if (VK_SUCCESS != vkEndCommandBuffer(cmd_buffer_load_tex)) {
        vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);
        return false;
    }

    SubmitCommandBufferAndWait(cmd_buffer_load_tex);

    vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);

    return true;
}

bool VkDemo::LoadCubeMaps(const char* filename,
    VkFormat format, VkImageUsageFlags image_usage,
    VkSampler sampler, VkImageLayout image_layout, vk_image_s& vk_image) {

/*
            _________
            |       |
            | left  |
     _______|_______|_______________
    |       |       |       |       |
    | back  | bottom| front | top   |
    |_______|_______|_______|_______|
            |       |
            | right |
            |_______|

    // order: front, back, left, right, bottom, top
*/


    memset(&vk_image, 0, sizeof(vk_image));

    char full_filename[MAX_PATH];

    if (filename[0] == '/' || filename[1] == ':') {
        Str_Copy(full_filename, MAX_PATH, filename);    // absolute push
    }
    else {
        // relative path, load from textures folder
        Str_SPrintf(full_filename, MAX_PATH, "%s/%s", textures_dir_, filename);
    }

    image_s pic = {};
    if (!Img_Load(full_filename, pic)) {
        printf("Failed to load texture \"%s\"\n", full_filename);
        return false;
    }

    if (pic.format_ != image_format_t::R8G8B8A8) {
        Img_Free(pic);
        printf("\"%s\": only support R8G8B8A8\n", full_filename);
        return false;
    }

    uint32_t face_w = pic.width_ / 4;
    uint32_t face_h = pic.height_ / 3;

    if (face_w != face_h) {
        Img_Free(pic);
        printf("\"%s\": not a cube map\n", full_filename);
        return false;
    }

    size_t buf_sz = face_w * face_h * 4 * 6;

    vk_buffer_s vk_buf = {};
    if (!CreateBuffer(vk_buf, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf_sz))
    {
        Img_Free(pic);
        printf("\"%s\": could not allocate buffer\n", full_filename);
        return false;
    }

    void* vk_buf_ptr = MapBuffer(vk_buf);
    if (!vk_buf_ptr) {
        DestroyBuffer(vk_buf);
        Img_Free(pic);
        printf("\"%s\": map buffer error\n", full_filename);
        return false;
    }

    byte_t* dst = (byte_t*)vk_buf_ptr;

    // pair: x offset, y offset
    std::array<std::pair<uint32_t, uint32_t>, 6> face_offsets = {
        std::pair<uint32_t, uint32_t>(face_w * 2, face_h * 1),
        std::pair<uint32_t, uint32_t>(face_w * 0, face_h * 1),
        std::pair<uint32_t, uint32_t>(face_w * 1, face_h * 0),
        std::pair<uint32_t, uint32_t>(face_w * 1, face_h * 2),
        std::pair<uint32_t, uint32_t>(face_w * 1, face_h * 1),
        std::pair<uint32_t, uint32_t>(face_w * 3, face_h * 1)
    };

    for (auto& it : face_offsets) {
         
        for (uint32_t y = 0; y < face_h; ++y) {
            byte_t* src_line = pic.pixels_ + (pic.width_ * (it.second + y) + it.first) * 4;
            byte_t* dst_line = dst + face_w * 4 * y;
            memcpy(dst_line, src_line, face_w * 4);
        }

        dst += (face_w * face_h * 4);
    }

    UnmapBuffer(vk_buf);

    // specify VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT for cube maps
    // add VK_IMAGE_USAGE_TRANSFER_DST_BIT flag to use vkCmdCopyBufferToImage command

    if (Create2DImage(vk_image, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        format, VK_IMAGE_TILING_OPTIMAL, image_usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, face_w, face_h, 6 /* cube map has 6 faces */,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE,
        sampler,
        image_layout)) {


        VkCommandPool command_pool = vk_command_pool_transient_;
        VkCommandBuffer cmd_buffer_load_tex = VK_NULL_HANDLE;

        VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};

        cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buffer_alloc_info.pNext = nullptr;
        cmd_buffer_alloc_info.commandPool = command_pool;
        cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buffer_alloc_info.commandBufferCount = 1;

        if (VK_SUCCESS != vkAllocateCommandBuffers(vk_device_, &cmd_buffer_alloc_info, &cmd_buffer_load_tex)) {
            DestroyImage(vk_image);
            DestroyBuffer(vk_buf);
            Img_Free(pic);
            return false;
        }

        VkCommandBufferBeginInfo cmdbuf_begin_info = {};

        cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbuf_begin_info.pNext = nullptr;
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdbuf_begin_info.pInheritanceInfo = nullptr;

        if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buffer_load_tex, &cmdbuf_begin_info)) {
            vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);
            DestroyImage(vk_image);
            DestroyBuffer(vk_buf);
            Img_Free(pic);
            return false;
        }


        std::array<VkBufferImageCopy, 6> buffer_image_copy_array;

        for (uint32_t i = 0; i < 6; ++i) {
            buffer_image_copy_array[i].bufferOffset = i * face_w * face_h * 4;
            buffer_image_copy_array[i].bufferRowLength = 0;
            buffer_image_copy_array[i].bufferImageHeight = 0;
            buffer_image_copy_array[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            buffer_image_copy_array[i].imageSubresource.mipLevel = 0;
            buffer_image_copy_array[i].imageSubresource.baseArrayLayer = i;
            buffer_image_copy_array[i].imageSubresource.layerCount = 1;
            buffer_image_copy_array[i].imageOffset.x = 0;
            buffer_image_copy_array[i].imageOffset.y = 0;
            buffer_image_copy_array[i].imageOffset.z = 0;
            buffer_image_copy_array[i].imageExtent.width = face_w;
            buffer_image_copy_array[i].imageExtent.height = face_h;
            buffer_image_copy_array[i].imageExtent.depth = 1;
        }


        VkImageMemoryBarrier image_memory_barrier = {};

        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = vk_image.image_;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 6;   // 6 faces
        SetAccessMaskOfImageMemoryBarrier(image_memory_barrier);

        vkCmdPipelineBarrier(cmd_buffer_load_tex,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0 /* dependencyFlags */,
            0 /* memoryBarrierCount */, nullptr /* pMemoryBarriers */,
            0 /* bufferMemoryBarrierCount */, nullptr /* pBufferMemoryBarries */,
            1, &image_memory_barrier);

        vkCmdCopyBufferToImage(
            cmd_buffer_load_tex,
            vk_buf.buffer_,
            vk_image.image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            6,
            buffer_image_copy_array.data());

        // set image layout to image_layout
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.newLayout = image_layout;
        SetAccessMaskOfImageMemoryBarrier(image_memory_barrier);

        vkCmdPipelineBarrier(cmd_buffer_load_tex,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0 /* dependencyFlags */,
            0 /* memoryBarrierCount */, nullptr /* pMemoryBarriers */,
            0 /* bufferMemoryBarrierCount */, nullptr /* pBufferMemoryBarries */,
            1, &image_memory_barrier);


        if (VK_SUCCESS != vkEndCommandBuffer(cmd_buffer_load_tex)) {
            vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);
            DestroyImage(vk_image);
            DestroyBuffer(vk_buf);
            Img_Free(pic);
            return false;
        }

        SubmitCommandBufferAndWait(cmd_buffer_load_tex);

        vkFreeCommandBuffers(vk_device_, command_pool, 1, &cmd_buffer_load_tex);
        DestroyBuffer(vk_buf);
        Img_Free(pic);

        return true;
    }
    else {
        DestroyBuffer(vk_buf);
        Img_Free(pic);
        return false;
    }
}

void VkDemo::DestroyFramebuffer(VkFramebuffer& vk_framebuffer) {
    if (vk_framebuffer) {
        vkDestroyFramebuffer(vk_device_, vk_framebuffer, nullptr);
        vk_framebuffer = VK_NULL_HANDLE;
    }
}

void VkDemo::DestroyRenderPass(VkRenderPass& vk_render_pass) {
    if (vk_render_pass) {
        vkDestroyRenderPass(vk_device_, vk_render_pass, nullptr);
        vk_render_pass = VK_NULL_HANDLE;
    }
}

void VkDemo::AddAdditionalInstanceExtensions(std::vector<const char*>& extensions) const {
    // to override by child class
}

void VkDemo::AddAdditionalDeviceExtensions(std::vector<const char*>& extensions) const {
    // to override by child class
}

void VkDemo::SubmitCommandBufferAndWait(VkCommandBuffer command_buffer) {
    VkFence temp_fence = VK_NULL_HANDLE;

    VkFenceCreateInfo fence_create_info = {};

    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = 0;

    if (VK_SUCCESS != vkCreateFence(vk_device_, &fence_create_info, nullptr, &temp_fence)) {
        printf("vkCreateFence error\n");
        return;
    }

    VkSubmitInfo queue_submit_info = {};

    queue_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queue_submit_info.pNext = nullptr;
    queue_submit_info.waitSemaphoreCount = 0;
    queue_submit_info.pWaitSemaphores = nullptr;
    queue_submit_info.pWaitDstStageMask = nullptr;
    queue_submit_info.commandBufferCount = 1;
    queue_submit_info.pCommandBuffers = &command_buffer;
    queue_submit_info.signalSemaphoreCount = 0;
    queue_submit_info.pSignalSemaphores = nullptr;

    // sumit to graphics queue
    if (VK_SUCCESS != vkQueueSubmit(vk_graphics_queue_, 1, &queue_submit_info, temp_fence)) {
        vkDestroyFence(vk_device_, temp_fence, nullptr);
        printf("vkQueueSubmit error\n");
        return;
    }

    if (VK_SUCCESS != vkWaitForFences(vk_device_, 1, &temp_fence, VK_TRUE, UINT64_MAX)) {
        vkDestroyFence(vk_device_, temp_fence, nullptr);
        printf("vkWaitForFences error\n");
        return;
    }

    vkDestroyFence(vk_device_, temp_fence, nullptr);
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
        printf("load shader file \"%s\" failed.\n", fullfilename);
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

void VkDemo::GetProjMatrix(glm::mat4& proj_mat) const {
    proj_mat = glm::perspectiveRH_ZO(glm::radians(fovy_),
        (float)cfg_viewport_cx_ / cfg_viewport_cy_, z_near_, z_far_);
}

void VkDemo::GetViewMatrix(glm::mat4& view_mat) const {
    view_mat = glm::lookAt(camera_.pos_, camera_.target_, camera_.up_);
}

void VkDemo::GetModelMatrix(glm::mat4& model_mat) const {
    glm::mat scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale_));

    if (camera_mode_ == camera_mode_t::CM_MOVE_CAMERA) {
        model_mat = scale_mat;
    }
    else {
        model_mat = (*model_rotate_mat_) * scale_mat;
    }
}

VkFormat VkDemo::GetIdealDepthFormat() const {
    // from hightest precision to lowest
    std::array<VkFormat, 5> formats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto & format : formats) {
        VkFormatProperties format_props;
        vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_props);
        if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat VkDemo::GetIdealDepthStencilFormat() const {
    std::array<VkFormat, 3> formats = {
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM_S8_UINT
    };

    for (auto& format : formats) {
        VkFormatProperties format_props;
        vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_props);
        if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool VkDemo::CreatePipelineVertFrag(const create_pipeline_vert_frag_params_s& params,
    VkPipeline& pipeline) 
{
    // check vertex format
    std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;

    // position: <x, y, z>
    vertex_attribute_descriptions.push_back(
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        }
    );

    uint32_t next_location = 1;

    uint32_t stride = (uint32_t)sizeof(vertex_pos_normal_s);
    uint32_t normal_offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_);
    uint32_t color_offset = 0;
    uint32_t uv_offset = 0;
    uint32_t tangent_offset = 0;

    switch (params.vertex_format_) {
    case vertex_format_t::VF_POS:
        stride = (uint32_t)sizeof(vertex_pos_s);
        break;
    case vertex_format_t::VF_POS_COLOR:
        stride = (uint32_t)sizeof(vertex_pos_color_s);
        if (params.additional_vertex_fields_ & VERTEX_FIELD_COLOR) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_color_s, color_)
                }
            );
        }
        break;
    case vertex_format_t::VF_POS_NORMAL:
        stride = (uint32_t)sizeof(vertex_pos_normal_s);
        if (params.additional_vertex_fields_ & VERTEX_FIELD_NORMAL) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_)
                }
            );
        }
        break;
    case vertex_format_t::VF_POS_UV:
        stride = (uint32_t)sizeof(vertex_pos_uv_s);
        if (params.additional_vertex_fields_ & VERTEX_FIELD_UV) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_uv_s, uv_)
                }
            );
        }
        break;
    case vertex_format_t::VF_POS_NORMAL_COLOR:
        stride = (uint32_t)sizeof(vertex_pos_normal_color_s);
        
        if (params.additional_vertex_fields_ & VERTEX_FIELD_NORMAL) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_color_s, normal_)
                }
            );
        }

        if (params.additional_vertex_fields_ & VERTEX_FIELD_COLOR) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_color_s, color_)
                }
            );
        }

        break;
    case vertex_format_t::VF_POS_NORMAL_UV:
        stride = (uint32_t)sizeof(vertex_pos_normal_uv_s);

        if (params.additional_vertex_fields_ & VERTEX_FIELD_NORMAL) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_uv_s, normal_)
                }
            );
        }

        if (params.additional_vertex_fields_ & VERTEX_FIELD_UV) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_uv_s, uv_)
                }
            );
        }

        break;
    case vertex_format_t::VF_POS_NORMAL_UV_TANGENT:
        stride = (uint32_t)sizeof(vertex_pos_normal_uv_tangent_s);

        if (params.additional_vertex_fields_ & VERTEX_FIELD_NORMAL) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_uv_tangent_s, normal_)
                }
            );
        }

        if (params.additional_vertex_fields_ & VERTEX_FIELD_UV) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_uv_tangent_s, uv_)
                }
            );
        }

        if (params.additional_vertex_fields_ & VERTEX_FIELD_TANGENT) {
            vertex_attribute_descriptions.push_back(
                {
                    .location = next_location++,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = GET_FIELD_OFFSET(vertex_pos_normal_uv_tangent_s, tangent_)
                }
            );
        }

        break;
    default:
        printf("Unknown vertex format %d\n", (int)params.vertex_format_);
        return false;
    }

    if (params.instance_format_ == instance_format_t::INST_POS_VEC3) {
        vertex_attribute_descriptions.push_back(
            {
                .location = next_location++,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            }
        );
        vertex_attribute_descriptions.push_back(
            {
                .location = next_location++,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = GET_FIELD_OFFSET(instance_pos_vec3_s, vec3_)
            }
        );
    }
    else if (params.instance_format_ == instance_format_t::INST_POS_VEC4) {
        vertex_attribute_descriptions.push_back(
            {
                .location = next_location++,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            }
        );
        vertex_attribute_descriptions.push_back(
            {
                .location = next_location++,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = GET_FIELD_OFFSET(instance_pos_vec4_s, vec4_)
            }
        );
    }

    // load shader
    VkShaderModule vert_shader = VK_NULL_HANDLE, frag_shader = VK_NULL_HANDLE;

    LoadShader(params.vertex_shader_filename_, vert_shader);
    LoadShader(params.framgment_shader_filename_, frag_shader);

    if (!vert_shader || !frag_shader) {
        if (vert_shader) {
            vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
        }

        if (frag_shader) {
            vkDestroyShaderModule(vk_device_, frag_shader, nullptr);
        }
        return false;
    }

    VkGraphicsPipelineCreateInfo create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;

    std::array<VkPipelineShaderStageCreateInfo, 2> stages;

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].pNext = nullptr;
    stages[0].flags = 0;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_shader;
    stages[0].pName = "main";
    stages[0].pSpecializationInfo = nullptr;

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].pNext = nullptr;
    stages[1].flags = 0;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_shader;
    stages[1].pName = "main";
    stages[1].pSpecializationInfo = nullptr;

    create_info.stageCount = (uint32_t)stages.size();
    create_info.pStages = stages.data();

    // -- pVertexInputState --

    std::vector<VkVertexInputBindingDescription> vertex_binding_description;

    vertex_binding_description.push_back(
        {
            .binding = 0,	// binding point 0
            .stride = stride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX	// per-vertex rate
        }
    );

    if (params.instance_format_ == instance_format_t::INST_POS_VEC3) {
        vertex_binding_description.push_back(
            {
                .binding = 1,	// binding point 1
                .stride = (uint32_t)sizeof(instance_pos_vec3_s),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE	// per-instance rate
            }
        );
    }
    else if (params.instance_format_ == instance_format_t::INST_POS_VEC4) {
        vertex_binding_description.push_back(
            {
                .binding = 1,	// binding point 1
                .stride = (uint32_t)sizeof(instance_pos_vec4_s),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE	// per-instance rate
            }
        );
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)vertex_binding_description.size(),
        .pVertexBindingDescriptions = vertex_binding_description.data(),
        .vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descriptions.size(),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
    };

    create_info.pVertexInputState = &vertex_input_state;

    // -- pInputAssemblyState --

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = params.primitive_topology_,
        .primitiveRestartEnable = params.primitive_restart_enable_
    };
    create_info.pInputAssemblyState = &input_assembly_state;

    // -- pTessellationState --
    create_info.pTessellationState = nullptr;   // no tessellation

    // -- pViewportState --
    VkPipelineViewportStateCreateInfo viewport_state = {};

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = nullptr;    // overriden by dynamic state
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = nullptr;     // overriden by dynamic state

    create_info.pViewportState = &viewport_state;

    // -- pRasterizationState --
    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = params.polygon_mode_,
        .cullMode = params.cull_mode_,
        .frontFace = params.front_face_,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    // glPolygonOffset(OffsetFactor, OffsetUnit); 

    create_info.pRasterizationState = &rasterization_state;

    // -- pMultisampleState --
    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    create_info.pMultisampleState = &multisample_state;

    // -- pDepthStencilState --

    VkStencilOpState stencil_op_state = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .compareMask = 0,
        .writeMask = 0,
        .reference = 0
    };

    // If the VK_EXT_depth_range_unrestricted extension is not enabled, minDepth must be between 0.0 and 1.0
    VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = params.depth_test_enable_,
        .depthWriteEnable = params.depth_write_enable_,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,  // pass condition: <=
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = stencil_op_state,
        .back = stencil_op_state,
        .minDepthBounds = 0.0,
        .maxDepthBounds = 1.0
    };

    create_info.pDepthStencilState = &depthstencil_state;

    // -- pColorBlendState --
    VkPipelineColorBlendStateCreateInfo color_blend_state = {};

    VkPipelineColorBlendAttachmentState blend_attr_attachment = {};
    blend_attr_attachment.colorWriteMask = 0xf;
    blend_attr_attachment.blendEnable = VK_FALSE;

    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.pNext = nullptr;
    color_blend_state.flags = 0;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &blend_attr_attachment;

    create_info.pColorBlendState = &color_blend_state;

    // -- pDynamicState --

    VkPipelineDynamicStateCreateInfo dynamic_state = {};

    std::vector<VkDynamicState> dynamic_state_enables;
    dynamic_state_enables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamic_state_enables.push_back(VK_DYNAMIC_STATE_SCISSOR);

    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = nullptr;
    dynamic_state.flags = 0;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());
    dynamic_state.pDynamicStates = dynamic_state_enables.data();

    create_info.pDynamicState = &dynamic_state;

    create_info.layout = params.pipeline_layout_;
    create_info.renderPass = params.render_pass_;

    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = 0;

    VkResult rt = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
        1, &create_info, nullptr, &pipeline);

    // free shader modules
    vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
    vkDestroyShaderModule(vk_device_, frag_shader, nullptr);

    return rt == VK_SUCCESS;
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

void VkDemo::CursorRotate(float delta_yaw, float delta_pitch) {
    if (camera_mode_ == camera_mode_t::CM_MOVE_CAMERA) {

        if (camera_rotation_flags_ & ROTATION_YAW_BIT) {
            view_angles_[ANGLE_YAW] += delta_yaw * 0.5f;
        }

        if (camera_rotation_flags_ & ROTATION_PITCH_BIT) {
            view_angles_[ANGLE_PITCH] += delta_pitch * 0.5f;
        }

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
    else {
        // quaternion rotation
        
        if (!(camera_rotation_flags_ & ROTATION_YAW_BIT)) {
            delta_yaw = 0.0f;
        }

        if (!(camera_rotation_flags_ & ROTATION_PITCH_BIT)) {
            delta_pitch = 0.0f;
        }

        glm::quat yaw_quat = glm::angleAxis(glm::radians(delta_yaw), glm::vec3(0.f, 0.f, 1.f));
        glm::quat pitch_quat = glm::angleAxis(-glm::radians(delta_pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // convert quaternion to matrix
        glm::mat4 delta_rotate_mat = glm::mat4_cast(pitch_quat * yaw_quat);

        // concatenate rotation
        glm::mat4 cur_model_rotate_mat = *model_rotate_mat_;
        *model_rotate_mat_ = delta_rotate_mat * cur_model_rotate_mat;
    }
}

void VkDemo::FuncKeyDown(uint32_t key) {
    // to override
}

// helper

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

            CursorRotate(delta_yaw, delta_pitch);

            cursor_x_ = new_cursor_x;
            cursor_y_ = new_cursor_y;

            // update window
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
        case VK_F10:
        case VK_F11:
        case VK_F12:
        case VK_PRIOR:
        case VK_NEXT:
            FuncKeyDown(wParam);
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

        /*
        uint32_t ext_pro_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_pro_count, nullptr);

        std::vector<VkExtensionProperties> ext_pros(ext_pro_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_pro_count, ext_pros.data());

        printf("-- device extentions --\n");
        for (auto& pro : ext_pros) {
            printf("%s\n", pro.extensionName);
        }
        */

        uint32_t queue_family_property_count = 0;
        /* void */ vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, queue_family_properties.data());

        bool support_graphics = false;

        for (uint32_t j = 0; j < queue_family_property_count; ++j) {
            VkQueueFamilyProperties& p = queue_family_properties[j];
            if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) { // that queue family support graphics
                
                // implicitly support VK_QUEUE_TRANSFER_BIT operator
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
            printf("minUniformBufferOffsetAlignment = %llu\n", vk_physical_device_properties2_.properties.limits.minUniformBufferOffsetAlignment);
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

    printf("Surfacecapabilities.minImageCount = %u\n", surf_caps.minImageCount);
    printf("Surfacecapabilities.maxImageCount = %u\n", surf_caps.maxImageCount);
    printf("Surfacecapabilities.currentExtent = (%u,%u)\n",
        surf_caps.currentExtent.width, surf_caps.currentExtent.height);
    printf("Surfacecapabilities.minImageExtent = (%u,%u)\n", 
        surf_caps.minImageExtent.width, surf_caps.minImageExtent.height);
    printf("Surfacecapabilities.maxImageExtent = (%u,%u)\n",
        surf_caps.maxImageExtent.width, surf_caps.maxImageExtent.height);

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
    VkExtent2D swapchain_image_extent = {};

    if (surf_caps.currentExtent.width == (uint32_t)-1) {
        // special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that the surface size
        // will be determined by the extent of a swapchain targeting the surface.

        swapchain_image_extent.width = cfg_viewport_cx_;
        swapchain_image_extent.height = cfg_viewport_cy_;
    }
    else {
        swapchain_image_extent = surf_caps.currentExtent;

        cfg_viewport_cx_ = swapchain_image_extent.width;
        cfg_viewport_cy_ = swapchain_image_extent.height;
    }

    // -- image_usage --
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Set additional usage flag for blitting from the swapchain images if supported. 
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(vk_physical_device_, image_format, &format_props);   // return void
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        // image clear operation
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    //if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {

    //}

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

    printf("Selected present_mode = %s\n", Vk_PresentModelToStr(present_mode));

    // fill create info

    VkSwapchainCreateInfoKHR create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.surface = vk_surface_;
    create_info.minImageCount = desired_swapchain_image_count;
    create_info.imageFormat = image_format;
    create_info.imageColorSpace = image_color_space;
    create_info.imageExtent = swapchain_image_extent;

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
        DestroyFramebuffer(vk_framebuffers_[i]);
    }
    vk_framebuffer_count_ = 0;
}

// command pool
bool VkDemo::CreateCommandPools() {
    auto CreateCommandPool = [this](VkCommandPoolCreateFlags flags, VkCommandPool& command_pool) -> bool {
        VkCommandPoolCreateInfo command_pool_create_info = {};

        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = nullptr;
        command_pool_create_info.flags = flags;
            // command buffers from this pool can only be submitted no queues 
            //   corresponding to this queue family.
        command_pool_create_info.queueFamilyIndex = vk_physical_device_graphics_queue_family_index_;

        return VK_SUCCESS == vkCreateCommandPool(vk_device_, &command_pool_create_info, nullptr, &command_pool);
    };

    return CreateCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, vk_command_pool_)
        && CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, vk_command_pool_transient_);
}

void VkDemo::DestroyCommandPools() {
    auto DestroyCommandPool = [this](VkCommandPool & command_pool) {
        if (command_pool) {
            vkDestroyCommandPool(vk_device_, command_pool, nullptr);
            command_pool = VK_NULL_HANDLE;
        }
    };

    DestroyCommandPool(vk_command_pool_transient_);
    DestroyCommandPool(vk_command_pool_);
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
    if (!enable_display_) {
        // not inited
        return;
    }

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

    // notify child class that window resized
    WindowSizeChanged();

    // rebuld command buffer
    BuildCommandBuffers();

    enable_display_ = true;
}


