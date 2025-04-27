#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec3 out_pos;		// in world space
layout (location = 1) out vec3 out_normal_in_local;
layout (location = 2) out vec3 out_normal_in_world;

void main() {
    gl_Position = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model * vec4(in_pos, 1.0);
    out_pos = (ubo_mvp.model * vec4(in_pos, 1.0)).xyz;
	out_normal_in_local = in_normal;
    out_normal_in_world = (ubo_mvp.model * vec4(in_normal, 0.0)).xyz;
}
