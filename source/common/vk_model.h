/******************************************************************************
 model
 *****************************************************************************/

#pragma once

/*
================================================================================
VkModel
================================================================================
*/
class COMMON_API VkModel {
public:

	VkModel(VkDemo * owner);
	~VkModel();

	bool					Load(const char * filename, bool move_to_origin);
	void					Free();

	vertex_format_t			GetVertexFormat() const;
	const float *			GetMin() const;
	const float *			GetMax() const;

	void					Cmd_BindBuffers(VkCommandBuffer cmd_buf);
	void					Cmd_Draw(VkCommandBuffer cmd_buf);

private:

	VkDemo *				owner_;

	vk_buffer_s				vertex_buffer_;
	vk_buffer_s				index_buffer_;
	uint32_t				index_count_;

	vertex_format_t			vertex_format_;
	float					min_[3];
	float					max_[3];
	

};
