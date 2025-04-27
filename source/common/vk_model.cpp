/******************************************************************************
 model
 *****************************************************************************/

#include "inc.h"

/*
================================================================================
helper
================================================================================
*/
template <class VERTEX_CLASS>
bool Template_CreateVertexBuffer(VkDemo* owner, const model_s & model, vk_buffer_s & buffer) {

	size_t vertex_buffer_size = sizeof(VERTEX_CLASS) * model.num_vertex_;

	return owner->CreateVertexBuffer(buffer, model.vertices_, vertex_buffer_size, true);
}


/*
================================================================================
VkModel
================================================================================
*/
VkModel::VkModel(VkDemo* owner):
	owner_(owner),
	vk_sampler_(VK_NULL_HANDLE),
	index_count_(0),
	vertex_format_(vertex_format_t::VF_POS_NORMAL),
	vk_materials_(nullptr),
	parts_(nullptr),
	material_count_(0),
	part_count_(0)
{
	memset(&vertex_buffer_, 0, sizeof(vertex_buffer_));
	memset(&index_buffer_, 0, sizeof(index_buffer_));

	memset(min_, 0, sizeof(min_));
	memset(max_, 0, sizeof(max_));

	memset(&vk_buffer_ubo_materials_, 0, sizeof(vk_buffer_ubo_materials_));
}

VkModel::~VkModel() {
	Free();
}

bool VkModel::Load(const load_params_s& params, const char* filename, 
	bool move_to_origin, const glm::mat4* transform)
{
	Free();
	
	if (!owner_->CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, vk_sampler_)) {
		return false;
	}
	
	model_s model = {};
	if (!owner_->LoadModel(filename, move_to_origin, model, transform)) {
		return false;
	}

	if (model.num_parts_ < 1) {
		Model_Free(model);
		return false;
	}

	parts_ = (vk_model_part_s*)TEMP_ALLOC(sizeof(vk_model_part_s) * model.num_parts_);
	if (!parts_) {
		return false;
	}

	part_count_ = model.num_parts_;
	for (uint32_t i = 0; i < model.num_parts_; ++i) {
		const model_part_s& src_part = model.parts_[i];
		vk_model_part_s& dst_part = parts_[i];

		dst_part.material_idx_ = src_part.material_idx_;
		dst_part.index_offset_ = src_part.index_offset_;
		dst_part.index_count_ = src_part.index_count_;
	}

	index_count_ = model.num_index_;
	vertex_format_ = model.vertex_format_;

	min_[0] = model.min_[0];
	min_[1] = model.min_[1];
	min_[2] = model.min_[2];

	max_[0] = model.max_[0];
	max_[1] = model.max_[1];
	max_[2] = model.max_[2];

	bool ok = false;

	switch (model.vertex_format_) {
	case vertex_format_t::VF_POS:
		ok = Template_CreateVertexBuffer<vertex_pos_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_COLOR:
		ok = Template_CreateVertexBuffer<vertex_pos_color_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL:
		ok = Template_CreateVertexBuffer<vertex_pos_normal_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_UV:
		ok = Template_CreateVertexBuffer<vertex_pos_uv_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_COLOR:
		ok = Template_CreateVertexBuffer<vertex_pos_normal_color_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV:
		ok = Template_CreateVertexBuffer<vertex_pos_normal_uv_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV_TANGENT:
		ok = Template_CreateVertexBuffer<vertex_pos_normal_uv_tangent_s>(owner_, model, vertex_buffer_);
		break;
	default:
		printf("Unknown vertex format %d\n", model.vertex_format_);
		break;
	}

	if (ok) {
		size_t index_buffer_size = sizeof(uint32_t) * index_count_;
		ok = owner_->CreateIndexBuffer(index_buffer_, model.indices_, index_buffer_size, true);
	}

	if (ok) {

		if (model.num_material_) {
			material_count_ = model.num_material_;

			size_t aligned_sz_of_ubo_mat = owner_->GetAlignedMinOffsetSize(sizeof(ubo_material_s));

			vk_materials_ = (vk_material_s*)TEMP_ALLOC(sizeof(vk_material_s) * material_count_);
			if (!vk_materials_) {
				return false;
			}
			memset(vk_materials_, 0, sizeof(vk_material_s) * material_count_);

			if (!owner_->CreateBuffer(vk_buffer_ubo_materials_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				aligned_sz_of_ubo_mat * model.num_material_)) {
				return false;
			}

			size_t size_of_material_buf = aligned_sz_of_ubo_mat * model.num_material_;
			char * host_material_buf = (char*)TEMP_ALLOC(size_of_material_buf);
			if (!host_material_buf) {
				return false;
			}

			for (uint32_t i = 0; i < model.num_material_; ++i) {
				const model_material_s* src = model.materials_ + i;
				ubo_material_s* dst = (ubo_material_s*)(host_material_buf + aligned_sz_of_ubo_mat * i);

				dst->ambient_ = glm::vec4(src->ambient_, 1.0f);
				dst->diffuse_ = glm::vec4(src->diffuse_, 1.0f);
				dst->specular_ = glm::vec4(src->specular_, 1.0f);
				dst->shininess_ = src->shiness_;
				dst->alpha_ = src->alpha_;

				if (src->tex_file_[0]) {
					// load texture
					owner_->Load2DTexture(src->tex_file_,
						VK_FORMAT_R8G8B8A8_UNORM,
						VK_IMAGE_USAGE_SAMPLED_BIT,
						vk_sampler_,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						vk_materials_[i].vk_texture_);
				}
			}

			// write material information to ubo buffer
			owner_->UpdateBuffer(vk_buffer_ubo_materials_,
				host_material_buf, size_of_material_buf);

			std::vector<VkDescriptorSetLayout> desc_set_layouts;
			desc_set_layouts.reserve(model.num_material_);
			for (uint32_t i = 0; i < model.num_material_; ++i) {
				desc_set_layouts.push_back(params.desc_set_layout_bind0_mat_bind1_tex_);
			}

			VkDescriptorSetAllocateInfo allocate_info = {};

			allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocate_info.pNext = nullptr;
			allocate_info.descriptorPool = owner_->GetDescriptorPool();
			allocate_info.descriptorSetCount = (uint32_t)desc_set_layouts.size();
			allocate_info.pSetLayouts = desc_set_layouts.data();

			VkDescriptorSet * vk_materials_desc_sets = (VkDescriptorSet*)TEMP_ALLOC(sizeof(VkDescriptorSet) * model.num_material_);
			if (!vk_materials_desc_sets) {
				return false;
			}

			if (VK_SUCCESS != vkAllocateDescriptorSets(owner_->GetDevice(), &allocate_info, vk_materials_desc_sets)) {
				TEMP_FREE(vk_materials_desc_sets);
				return false;
			}

			VkDescriptorBufferInfo * desc_buffer_infos = (VkDescriptorBufferInfo*)TEMP_ALLOC(sizeof(VkDescriptorBufferInfo) * model.num_material_);

			std::vector<VkWriteDescriptorSet> write_descriptor_sets;

			for (uint32_t i = 0; i < model.num_material_; ++i) {
				vk_material_s* vk_mat = &vk_materials_[i];

				vk_mat->vk_desc_set_ = vk_materials_desc_sets[i];	// assign descriptor set

				desc_buffer_infos[i].buffer = vk_buffer_ubo_materials_.buffer_;
				desc_buffer_infos[i].offset = i * aligned_sz_of_ubo_mat;
				desc_buffer_infos[i].range = sizeof(ubo_material_s);

				write_descriptor_sets.push_back(
					{
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = vk_materials_desc_sets[i],
						.dstBinding = 0,	// binding = 0
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &desc_buffer_infos[i],
						.pTexelBufferView = nullptr
					}
				);

				if (vk_mat->vk_texture_.image_) {
					write_descriptor_sets.push_back(
						{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.pNext = nullptr,
							.dstSet = vk_materials_desc_sets[i],
							.dstBinding = 1,	// binding = 1
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
							.pImageInfo = &vk_mat->vk_texture_.desc_image_info_,
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr
						}
					);
				}
			}

			vkUpdateDescriptorSets(owner_->GetDevice(),
				(uint32_t)write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);

			TEMP_FREE(desc_buffer_infos);
			TEMP_FREE(vk_materials_desc_sets);
		}
	}

	Model_Free(model);

	return ok;
}

void VkModel::Free() {
	SAFE_FREE(parts_);
	part_count_ = 0;

	if (vk_materials_) {
		for (uint32_t i = 0; i < material_count_; ++i) {
			vk_material_s& vk_mat = vk_materials_[i];

			if (vk_mat.vk_desc_set_) {
				vkFreeDescriptorSets(owner_->GetDevice(), owner_->GetDescriptorPool(),
					1, &vk_mat.vk_desc_set_);
			}

			if (vk_mat.vk_texture_.image_) {
				owner_->DestroyImage(vk_mat.vk_texture_);
			}
		}
		TEMP_FREE(vk_materials_);
		vk_materials_ = nullptr;
	}

	material_count_ = 0;

	owner_->DestroyBuffer(index_buffer_);
	owner_->DestroyBuffer(vertex_buffer_);
	index_count_ = 0;
	memset(min_, 0, sizeof(min_));
	memset(max_, 0, sizeof(max_));

	owner_->DestroyBuffer(vk_buffer_ubo_materials_);

	owner_->DestroySampler(vk_sampler_);
}

vertex_format_t	VkModel::GetVertexFormat() const {
	return vertex_format_;
}

const float* VkModel::GetMin() const {
	return min_;
}

const float* VkModel::GetMax() const {
	return max_;
}

uint32_t VkModel::GetIndexCount() const {
	return index_count_;
}

VkBuffer VkModel::GetVertexBuffer() const {
	return vertex_buffer_.buffer_;
}

VkBuffer VkModel::GetIndexBuffer() const {
	return index_buffer_.buffer_;
}

uint32_t VkModel::GetMaterialCount() const {
	return material_count_;
}

const VkModel::vk_material_s* VkModel::GetMaterialByIdx(uint32_t idx) const {
	return vk_materials_ + idx;
}

uint32_t VkModel::GetModelPartCount() const {
	return part_count_;
}

const VkModel::vk_model_part_s* VkModel::GetModelPartByIdx(uint32_t idx) const {
	return parts_ + idx;
}
