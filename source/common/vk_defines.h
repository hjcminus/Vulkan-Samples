/******************************************************************************
 definitions
 *****************************************************************************/

#pragma once

/*
================================================================================
defines
================================================================================
*/
static const uint32_t       VK_INVALID_INDEX = (uint32_t)-1;

/*
================================================================================
uniform buffers
================================================================================
*/
struct alignas(16) ubo_mat_s {
    glm::mat4			    matrix_;
};

struct alignas(16) ubo_mvp_sep_s {
    glm::mat4			    proj_;
    glm::mat4			    view_;
    glm::mat4			    model_;
};

struct alignas(16) ubo_viewer_s {
    glm::vec4               pos_;
};

struct alignas(16) ubo_material_s {
    glm::vec4               ambient_;
    glm::vec4               diffuse_;
    glm::vec4               specular_;
    float                   shininess_;
    float                   alpha_;
};

struct alignas(16) ubo_point_light_s {
    glm::vec4			    pos_;
    glm::vec4			    color_;
    float                   radius_;
};

struct alignas(16) ubo_directional_light_s {
    glm::vec4			    dir_;
    glm::vec4			    color_;
};

/*
================================================================================
buffer
================================================================================
*/

// uniform buffer & shader storage buffer
struct vk_buffer_s {
    VkDeviceMemory		    memory_;
    VkDeviceSize		    memory_size_;
    VkBuffer			    buffer_;
    VkDescriptorBufferInfo  desc_buffer_info_;
};

struct vk_image_s {
    VkImage				    image_;
    uint32_t                width_;
    uint32_t                height_;
    VkDeviceMemory		    memory_;
    VkDeviceSize		    memory_size_;
    VkImageView			    image_view_;
    VkDescriptorImageInfo   desc_image_info_;
};
