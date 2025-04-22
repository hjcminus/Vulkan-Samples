#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (set = 3, binding = 0) uniform UBO_MODEL {
	mat4 matrix;
} ubo_model;

// bind 0
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

// bind 1
layout (location = 3) in vec3 in_inst_pos;
layout (location = 4) in vec3 in_inst_scale;

layout (location = 0) out vec4 out_pos;
layout (location = 1) out vec3 out_pos_in_view;
layout (location = 2) out vec3 out_pos_in_world;
layout (location = 3) out vec3 out_normal_in_world;
layout (location = 4) out vec2 out_uv;

void main() {
	vec3 new_pos;
	
	new_pos.x = in_pos.x * in_inst_scale.x;
	new_pos.y = in_pos.y * in_inst_scale.y;
	new_pos.z = in_pos.z * in_inst_scale.z;

	new_pos = (ubo_model.matrix * vec4(new_pos, 1.0)).xyz;

	new_pos += in_inst_pos;

	out_pos_in_view = (ubo_mvp.view * ubo_mvp.model * vec4(new_pos, 1.0)).xyz;

	mat4 mvp = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model;

	out_pos = mvp * vec4(new_pos, 1.0);
	gl_Position = out_pos;

    out_pos_in_world = (ubo_mvp.model * vec4(new_pos, 1.0)).xyz;
	
	vec4 new_normal = ubo_model.matrix * vec4(in_normal, 0.0);
	out_normal_in_world = (ubo_mvp.model * new_normal).xyz;

	out_uv = in_uv;
}
