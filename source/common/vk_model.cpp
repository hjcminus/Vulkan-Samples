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

bool CreateVertexBuffer(VkDemo* owner, const model_s & model, vk_buffer_s & buffer) {

	size_t vertex_buffer_size = sizeof(VERTEX_CLASS) * model.num_vertex_;

	if (!owner->CreateBuffer(buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertex_buffer_size)) {
		return false;
	}

	void* data = owner->MapMemory(buffer);
	if (data) {
		memcpy(data, model.vertices_, vertex_buffer_size);
	}

	owner->UnmapMemory(buffer);

	return true;
}


/*
================================================================================
VkModel
================================================================================
*/
VkModel::VkModel(VkDemo* owner):
	owner_(owner),
	index_count_(0),
	vertex_format_(vertex_format_t::VF_POS_NORMAL)
{
	memset(&vertex_buffer_, 0, sizeof(vertex_buffer_));
	memset(&index_buffer_, 0, sizeof(index_buffer_));

	memset(min_, 0, sizeof(min_));
	memset(max_, 0, sizeof(max_));
}

VkModel::~VkModel() {
	Free();
}

bool VkModel::Load(const char* filename, bool move_to_origin) {
	model_s model = {};
	if (!owner_->LoadModel(filename, move_to_origin, model)) {
		return false;
	}

	index_count_ = model.num_triangle_ * 3;
	vertex_format_ = model.vertex_format_;

	min_[0] = model.min_[0];
	min_[1] = model.min_[1];
	min_[2] = model.min_[2];

	max_[0] = model.max_[0];
	max_[1] = model.max_[1];
	max_[2] = model.max_[2];

	bool ok = false;

	switch (model.vertex_format_) {
	case vertex_format_t::VF_POS_COLOR:
		ok = CreateVertexBuffer<vertex_pos_color_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL:
		ok = CreateVertexBuffer<vertex_pos_normal_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_COLOR:
		ok = CreateVertexBuffer<vertex_pos_normal_color_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV:
		ok = CreateVertexBuffer<vertex_pos_normal_uv_s>(owner_, model, vertex_buffer_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV_TANGENT:
		ok = CreateVertexBuffer<vertex_pos_normal_uv_tangent_s>(owner_, model, vertex_buffer_);
		break;
	default:
		printf("Unknown vertex format %d\n", model.vertex_format_);
		break;
	}

	if (ok) {
		size_t index_buffer_size = sizeof(uint32_t) * model.num_triangle_ * 3;

		ok = owner_->CreateBuffer(index_buffer_,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			index_buffer_size);

		if (ok) {
			void* data = owner_->MapMemory(index_buffer_);
			if (data) {
				memcpy(data, model.indices_, index_buffer_size);
			}
			else {
				ok = false;
			}

			owner_->UnmapMemory(index_buffer_);
		}

	}

	Model_Free(model);

	return ok;
}

void VkModel::Free() {
	owner_->DestroyBuffer(index_buffer_);
	owner_->DestroyBuffer(vertex_buffer_);
	index_count_ = 0;
	memset(min_, 0, sizeof(min_));
	memset(max_, 0, sizeof(max_));
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

void VkModel::Cmd_BindBuffers(VkCommandBuffer cmd_buf) {
	VkDeviceSize offset[1] = { 0 };
	vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer_.buffer_, offset);

	vkCmdBindIndexBuffer(cmd_buf, index_buffer_.buffer_, 0, VK_INDEX_TYPE_UINT32);
}

void VkModel::Cmd_Draw(VkCommandBuffer cmd_buf) {
	vkCmdDrawIndexed(cmd_buf, index_count_, 1, 0, 0, 1);
}

