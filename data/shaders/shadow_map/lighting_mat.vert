#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4	proj;
	mat4	view;
	mat4	model;
} ubo_mvp;

layout (binding = 1) uniform UBO_LIGHT_MVP {
	mat4	mvp;
} ubo_light_mvp;

layout (binding = 3) uniform UBO_DIR_LIGHT {
    vec4  dir;
    vec4  color;
} ubo_dir_light;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec4 out_pos_light_space;
layout (location = 3) out float out_cos_theta;

void main() {
	gl_Position = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model * vec4(in_pos, 1.0);
	out_pos = (ubo_mvp.model * vec4(in_pos, 1.0)).xyz;
	out_normal = (ubo_mvp.model * vec4(in_normal, 0.0)).xyz;
	out_pos_light_space = ubo_light_mvp.mvp * vec4(in_pos, 1.0);
	out_cos_theta = max(0.0, dot(out_normal, -ubo_dir_light.dir.xyz));
}
