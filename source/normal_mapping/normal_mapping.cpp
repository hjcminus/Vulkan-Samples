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
	memset(&uniform_buffer_light_, 0, sizeof(uniform_buffer_light_));

	memset(&vertex_buffer_, 0, sizeof(vertex_buffer_));
	memset(&index_buffer_, 0, sizeof(index_buffer_));
}

NormalMappingDemo::~NormalMappingDemo() {
	// do nothing
}

bool NormalMappingDemo::Init() {
	if (!VkDemo::Init("normal_mapping" /* shader files directory */)) {
		return false;
	}

	if (!CreateDescriptorPools(16, 0, 16, 16)) {
		return false;
	}

	if (!LoadTextures()) {
		return false;
	}

	if (!CreaetSampler()) {
		return false;
	}

	if (!CreateUniformBuffers()) {
		return false;
	}

	if (!CreateVertexBuffer()) {
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

	camera_.pos_ = glm::vec3(0.0f, -2.0f, 0.0f);
	camera_.target_ = glm::vec3(0.0f);
	camera_.up_ = glm::vec3(0.0f, 0.0f, 1.0f);

	move_speed_ = 0.1f;

	SetupLightUniformBuffer();

	SetRenderMode(render_mode_t::RM_NORMAL_MAPPING);

	enable_display_ = true;

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
	DestroyVertexBuffer();
	DestroyUniformBuffers();
	DestroySampler();
	FreeTextures();

	DestroyDescriptorPools();

	VkDemo::Shutdown();
}

void NormalMappingDemo::Display() {
	uint32_t current_image_idx = 0;
	VkResult rt;

	UpdateMVPUniformBuffer();

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

	// change command buffer state from executable to pending
	// The fence (optional) will be signaled once all submitted command buffers have completed execution.
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

void NormalMappingDemo::SetRenderMode(render_mode_t mode) {
	render_mode_ = mode;

	switch (mode) {
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

void NormalMappingDemo::KeyF2Down() {
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

bool NormalMappingDemo::LoadTexture(const char* filename, VkFormat format,
	VkImageUsageFlags image_usage, texture_s& tex)
{
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format, &format_properties);

	if (!(format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
		printf("linearTilingFeatures not support VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT\n");
		return false;
	}

	char full_filename[MAX_PATH];
	Str_SPrintf(full_filename, MAX_PATH, "%s/%s", textures_dir_, filename);

	image_s image = {};
	if (!Img_Load(full_filename, image)) {
		printf("Failed to load texture \"%s\"\n", full_filename);
		return false;
	}

	VkImageCreateInfo image_create_info = {};

	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = nullptr;
	image_create_info.flags = 0;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = format;
	image_create_info.extent.width = (uint32_t)image.width_;
	image_create_info.extent.height = (uint32_t)image.height_;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_create_info.usage = image_usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.queueFamilyIndexCount = 0;
	image_create_info.pQueueFamilyIndices = nullptr;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (VK_SUCCESS != vkCreateImage(vk_device_, &image_create_info, nullptr, &tex.image_)) {
		Img_Free(image);
		return false;
	}

	VkMemoryRequirements memory_requirements = {};
	vkGetImageMemoryRequirements(vk_device_, tex.image_, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = GetMemoryTypeIndex(
			memory_requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	};

	if (VK_SUCCESS != vkAllocateMemory(vk_device_, &memory_allocate_info, nullptr, &tex.memory_)) {
		Img_Free(image);
		return false;
	}

	if (VK_SUCCESS != vkBindImageMemory(vk_device_, tex.image_, tex.memory_, 0 /* memoryOffset */)) {
		Img_Free(image);
		return false;
	}

	void* mapped = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, tex.memory_, 0, memory_requirements.size, 0 /* flags */, &mapped)) {
		Img_Free(image);
		return false;
	}

	memcpy(mapped, image.pixels_, image.width_ * image.height_ * 4);

	vkUnmapMemory(vk_device_, tex.memory_);

	Img_Free(image);

	// load texture
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
	image_memory_barrier.image = tex.image_;
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

	// create image view
	VkImageViewCreateInfo image_view_create_info = {};
	
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext = nullptr;
	image_view_create_info.flags = 0;
	image_view_create_info.image = tex.image_;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = format;
	// components
	image_view_create_info.subresourceRange = image_memory_barrier.subresourceRange;

	if (VK_SUCCESS != vkCreateImageView(vk_device_, &image_view_create_info,
		nullptr, &tex.image_view_)) {
		return false;
	}

	return true;
}

void NormalMappingDemo::FreeTexture(texture_s& tex) {
	if (tex.image_view_) {
		vkDestroyImageView(vk_device_, tex.image_view_, nullptr);
		tex.image_view_ = VK_NULL_HANDLE;
	}

	if (tex.memory_) {
		vkFreeMemory(vk_device_, tex.memory_, nullptr);
		tex.memory_ = VK_NULL_HANDLE;
	}

	if (tex.image_) {
		vkDestroyImage(vk_device_, tex.image_, nullptr);
		tex.image_ = VK_NULL_HANDLE;
	}
}

bool NormalMappingDemo::LoadTextures() {
	if (!LoadTexture("color.bmp", VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, texture_color_)) {
		return false;
	}

	if (!LoadTexture("normal.bmp", VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT, texture_normal_)) {
		return false;
	}

	return true;
}

void NormalMappingDemo::FreeTextures() {
	FreeTexture(texture_normal_);
	FreeTexture(texture_color_);
}

bool NormalMappingDemo::CreaetSampler() {
	VkSamplerCreateInfo create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.magFilter = VK_FILTER_NEAREST;
	create_info.minFilter = VK_FILTER_LINEAR;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	create_info.mipLodBias = 0.0f;
	create_info.anisotropyEnable = VK_FALSE;
	create_info.maxAnisotropy = 0.0f;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_NEVER;
	create_info.minLod = 0.0f;
	create_info.maxLod = 1.0f;
	create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	create_info.unnormalizedCoordinates = VK_FALSE;

	return VK_SUCCESS == vkCreateSampler(vk_device_, &create_info, nullptr, &vk_sampler_);
}

void NormalMappingDemo::DestroySampler() {
	if (vk_sampler_) {
		vkDestroySampler(vk_device_, vk_sampler_, nullptr);
		vk_sampler_ = VK_NULL_HANDLE;
	}
}

bool NormalMappingDemo::CreateUniformBuffers() {
	return CreateBuffer(uniform_buffer_mat_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ubo_mat_s))
		&& CreateBuffer(uniform_buffer_light_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(uniform_buffer_light_));
}

void NormalMappingDemo::DestroyUniformBuffers() {
	DestroyBuffer(uniform_buffer_light_);
	DestroyBuffer(uniform_buffer_mat_);
}

bool NormalMappingDemo::CreateVertexBuffer() {
	std::vector<vertex_s> vertex_buffer = {
		{ { -1.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, 0.0f,  1.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { -1.0f, 0.0f,  1.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
	};
	uint32_t vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(vertex_s);

	if (!CreateBuffer(vertex_buffer_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_buffer_size)) {
		return false;
	}

	void* data = nullptr;
	if (VK_SUCCESS != vkMapMemory(vk_device_, vertex_buffer_.memory_, 0, vertex_buffer_.memory_size_, 0, &data)) {
		return false;
	}

	memcpy(data, vertex_buffer.data(), vertex_buffer_size);

	vkUnmapMemory(vk_device_, vertex_buffer_.memory_);

	return true;
}

void NormalMappingDemo::DestroyVertexBuffer() {
	DestroyBuffer(vertex_buffer_);
}

// index buffer
bool NormalMappingDemo::CreateIndexBuffer() {
	uint32_t index_buffer[6] = { 0, 1, 2, 2, 3, 0 };
	uint32_t index_buffer_size = static_cast<uint32_t>(sizeof(index_buffer));

	if (!CreateBuffer(index_buffer_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_buffer_size)) {
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

	VkDescriptorSetLayoutBinding binding[2] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = 2;
	create_info.pBindings = binding;

	return VK_SUCCESS == vkCreateDescriptorSetLayout(vk_device_, &create_info, nullptr, &vk_descriptorset_layout_uniform_);
}

bool NormalMappingDemo::CreateDescriptorSetLayout_Sampler() {
	VkDescriptorSetLayoutCreateInfo create_info = {};

	VkDescriptorSetLayoutBinding binding[2] = {
		{
			.binding = 0,	// the binding number of this entry and 
				// corresponds to a resource of the same binding number in the shader stages.
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,	// non-array

			// specifying which pipeline shader stages can access a resource for this binding.
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,	// non-array
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = 2;
	create_info.pBindings = binding;

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


	std::array<VkWriteDescriptorSet, 4> write_descriptor_sets;

	int idx = 0;
	write_descriptor_sets[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[idx].pNext = nullptr;
	write_descriptor_sets[idx].dstSet = vk_descriptorset_uniform_;
	write_descriptor_sets[idx].dstBinding = 0;
	write_descriptor_sets[idx].dstArrayElement = 0;
	write_descriptor_sets[idx].descriptorCount = 1;
	write_descriptor_sets[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[idx].pImageInfo = nullptr;
	write_descriptor_sets[idx].pBufferInfo = &uniform_buffer_mat_.buffer_info_;
	write_descriptor_sets[idx].pTexelBufferView = nullptr;

	idx++;
	write_descriptor_sets[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[idx].pNext = nullptr;
	write_descriptor_sets[idx].dstSet = vk_descriptorset_uniform_;
	write_descriptor_sets[idx].dstBinding = 1;
	write_descriptor_sets[idx].dstArrayElement = 0;
	write_descriptor_sets[idx].descriptorCount = 1;
	write_descriptor_sets[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[idx].pImageInfo = nullptr;
	write_descriptor_sets[idx].pBufferInfo = &uniform_buffer_light_.buffer_info_;
	write_descriptor_sets[idx].pTexelBufferView = nullptr;

	idx++;

	VkDescriptorImageInfo desc_image_info_color = {};

	desc_image_info_color.sampler = vk_sampler_;
	desc_image_info_color.imageView = texture_color_.image_view_;
	desc_image_info_color.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	write_descriptor_sets[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[idx].pNext = nullptr;
	write_descriptor_sets[idx].dstSet = vk_descriptorset_sampler_;
	write_descriptor_sets[idx].dstBinding = 0;
	write_descriptor_sets[idx].dstArrayElement = 0;
	write_descriptor_sets[idx].descriptorCount = 1;
	write_descriptor_sets[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[idx].pImageInfo = &desc_image_info_color;
	write_descriptor_sets[idx].pBufferInfo = nullptr;
	write_descriptor_sets[idx].pTexelBufferView = nullptr;

	idx++;

	VkDescriptorImageInfo desc_image_info_normal = {};

	desc_image_info_normal.sampler = vk_sampler_;
	desc_image_info_normal.imageView = texture_normal_.image_view_;
	desc_image_info_normal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	write_descriptor_sets[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[idx].pNext = nullptr;
	write_descriptor_sets[idx].dstSet = vk_descriptorset_sampler_;
	write_descriptor_sets[idx].dstBinding = 1;
	write_descriptor_sets[idx].dstArrayElement = 0;
	write_descriptor_sets[idx].descriptorCount = 1;
	write_descriptor_sets[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[idx].pImageInfo = &desc_image_info_normal;
	write_descriptor_sets[idx].pBufferInfo = nullptr;
	write_descriptor_sets[idx].pTexelBufferView = nullptr;

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
	VkShaderModule vert_shader = VK_NULL_HANDLE, frag_shader = VK_NULL_HANDLE;

	LoadShader("SPIR-V/tex.vert.spv", vert_shader);
	LoadShader("SPIR-V/tex.frag.spv", frag_shader);

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

	VkVertexInputBindingDescription vertex_binding_description = {};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = (uint32_t)sizeof(vertex_s);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertex_attribute_descriptions;

	// see vertex shader
	vertex_attribute_descriptions[0].location = 0;
	vertex_attribute_descriptions[0].binding = 0;
	vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[0].offset = GET_FIELD_OFFSET(vertex_s, pos_);

	vertex_attribute_descriptions[1].location = 1;
	vertex_attribute_descriptions[1].binding = 0;
	vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attribute_descriptions[1].offset = GET_FIELD_OFFSET(vertex_s, uv_);

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertex_binding_description,
		.vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descriptions.size(),
		.pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
	};

	create_info.pVertexInputState = &vertex_input_state;

	// -- pInputAssemblyState --

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

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

	VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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

	create_info.layout = vk_pipeline_layout_;
	create_info.renderPass = vk_render_pass_;

	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = 0;

	VkResult rt = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_tex_);

	// free shader modules
	vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
	vkDestroyShaderModule(vk_device_, frag_shader, nullptr);

	return rt == VK_SUCCESS;
}

bool NormalMappingDemo::CreatePipeline_Flat() {
	VkShaderModule vert_shader = VK_NULL_HANDLE, frag_shader = VK_NULL_HANDLE;

	LoadShader("SPIR-V/flat.vert.spv", vert_shader);
	LoadShader("SPIR-V/flat.frag.spv", frag_shader);

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

	VkVertexInputBindingDescription vertex_binding_description = {};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = (uint32_t)sizeof(vertex_s);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> vertex_attribute_descriptions;

	// see vertex shader

	// pos
	vertex_attribute_descriptions[0].location = 0;
	vertex_attribute_descriptions[0].binding = 0;
	vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[0].offset = GET_FIELD_OFFSET(vertex_s, pos_);

	// normal
	vertex_attribute_descriptions[1].location = 1;
	vertex_attribute_descriptions[1].binding = 0;
	vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[1].offset = GET_FIELD_OFFSET(vertex_s, normal_);

	// uv
	vertex_attribute_descriptions[2].location = 2;
	vertex_attribute_descriptions[2].binding = 0;
	vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attribute_descriptions[2].offset = GET_FIELD_OFFSET(vertex_s, uv_);

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertex_binding_description,
		.vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descriptions.size(),
		.pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
	};

	create_info.pVertexInputState = &vertex_input_state;

	// -- pInputAssemblyState --

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

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

	VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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

	create_info.layout = vk_pipeline_layout_;
	create_info.renderPass = vk_render_pass_;

	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = 0;

	VkResult rt = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_flat_);

	// free shader modules
	vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
	vkDestroyShaderModule(vk_device_, frag_shader, nullptr);

	return rt == VK_SUCCESS;
}

bool NormalMappingDemo::CreatePipeline_NormalMapping() {
	VkShaderModule vert_shader = VK_NULL_HANDLE, frag_shader = VK_NULL_HANDLE;

	LoadShader("SPIR-V/normal_mapping.vert.spv", vert_shader);
	LoadShader("SPIR-V/normal_mapping.frag.spv", frag_shader);

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

	VkVertexInputBindingDescription vertex_binding_description = {};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = (uint32_t)sizeof(vertex_s);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 4> vertex_attribute_descriptions;

	// see vertex shader

	// pos
	vertex_attribute_descriptions[0].location = 0;
	vertex_attribute_descriptions[0].binding = 0;
	vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[0].offset = GET_FIELD_OFFSET(vertex_s, pos_);

	// normal
	vertex_attribute_descriptions[1].location = 1;
	vertex_attribute_descriptions[1].binding = 0;
	vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[1].offset = GET_FIELD_OFFSET(vertex_s, normal_);

	// uv
	vertex_attribute_descriptions[2].location = 2;
	vertex_attribute_descriptions[2].binding = 0;
	vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attribute_descriptions[2].offset = GET_FIELD_OFFSET(vertex_s, uv_);

	// tangent
	vertex_attribute_descriptions[3].location = 3;
	vertex_attribute_descriptions[3].binding = 0;
	vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_descriptions[3].offset = GET_FIELD_OFFSET(vertex_s, tangent_);

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertex_binding_description,
		.vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descriptions.size(),
		.pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
	};

	create_info.pVertexInputState = &vertex_input_state;

	// -- pInputAssemblyState --

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

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

	VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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

	create_info.layout = vk_pipeline_layout_;
	create_info.renderPass = vk_render_pass_;

	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = 0;

	VkResult rt = vkCreateGraphicsPipelines(vk_device_, vk_pipeline_cache_,
		1, &create_info, nullptr, &vk_pipeline_normal_mapping_);

	// free shader modules
	vkDestroyShaderModule(vk_device_, vert_shader, nullptr);
	vkDestroyShaderModule(vk_device_, frag_shader, nullptr);

	return rt == VK_SUCCESS;
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
			.maxDepth = 0.0f
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
	ubo_mat_s ubo_mvp = {};

	ubo_mvp.mat_proj_ = glm::perspective(glm::radians(70.0f),
		(float)cfg_viewport_cx_ / cfg_viewport_cy_, 0.1f, 16.0f);

	GetViewMatrix(ubo_mvp.mat_view_);
	GetModelMatrix(ubo_mvp.mat_model_);

	UpdateUniformBuffer(uniform_buffer_mat_, (const std::byte*)&ubo_mvp, sizeof(ubo_mvp));
}

void NormalMappingDemo::SetupLightUniformBuffer() {
	ubo_light_s ubo_light = {};

	// point light
	ubo_light.light_pos_ = glm::vec4(2.0f, -2.0f, 2.0f, 1.0f);
	ubo_light.light_color_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	UpdateUniformBuffer(uniform_buffer_light_, (const std::byte*)&ubo_light, sizeof(ubo_light));
}

void NormalMappingDemo::UpdateUniformBuffer(buffer_s& buffer, const std::byte* host_data, size_t host_data_size) {
	void* data = nullptr;
	VkResult rt = vkMapMemory(vk_device_, buffer.memory_, buffer.buffer_info_.offset, buffer.buffer_info_.range, 0, &data);
	if (rt != VK_SUCCESS) {
		printf("[UpdateMVPUniformBuffer] vkMapMemory error\n");
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
		printf("[UpdateMVPUniformBuffer] vkFlushMappedMemoryRanges error\n");
	}

	vkUnmapMemory(vk_device_, buffer.memory_);
}


/*
================================================================================
entrance
================================================================================
*/
DEMO_MAIN(NormalMappingDemo)
