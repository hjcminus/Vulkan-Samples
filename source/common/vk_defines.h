/******************************************************************************
 definitions
 *****************************************************************************/

#pragma once

/*
================================================================================
uniform buffers
================================================================================
*/
struct alignas(16) ubo_mvp_s {
    glm::mat4			mvp_;
};

struct alignas(16) ubo_mvp_sep_s {
    glm::mat4			proj_;
    glm::mat4			view_;
    glm::mat4			model_;
};

struct alignas(16) ubo_viewer_s {
    glm::vec4           pos_;
};

struct alignas(16) ubo_material_s {
    glm::vec4           color_;
    float               shininess_;
};

struct alignas(16) ubo_light_s {
    glm::vec4			pos_;
    glm::vec4			color_;
    float               radius_;
};

/*
================================================================================
buffer
================================================================================
*/

// uniform buffer & shader storage buffer
struct vk_buffer_s {
    VkDeviceMemory		memory_;
    VkDeviceSize		memory_size_;
    VkBuffer			buffer_;
    VkDescriptorBufferInfo  buffer_info_;
};

struct vk_image_s {
    VkImage				image_;
    VkDeviceMemory		memory_;
    VkDeviceSize		memory_size_;
    VkImageView			image_view_;
};
