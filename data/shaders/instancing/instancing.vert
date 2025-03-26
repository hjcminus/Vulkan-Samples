#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

// bind 0
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

// bind 1
layout (location = 2) in vec3 in_inst_pos;
layout (location = 3) in vec4 in_inst_rgb_and_scale;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_color;

void main() {
	vec3 new_pos = in_pos * in_inst_rgb_and_scale.w;
	new_pos += in_inst_pos;

    gl_Position = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model * vec4(new_pos, 1.0);
    out_pos = (ubo_mvp.model * vec4(new_pos, 1.0)).xyz;
	out_normal = (ubo_mvp.model * vec4(in_normal, 0.0)).xyz;
	out_color = in_inst_rgb_and_scale.rgb;
}
