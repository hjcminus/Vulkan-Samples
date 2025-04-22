#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord2d;
layout (location = 3) in vec3 in_tangent;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_texcoord2d;
layout (location = 3) out vec3 out_tangent;

void main() {
    gl_Position = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model * vec4(in_pos, 1.0);
	out_pos = (ubo_mvp.model * vec4(in_pos, 1.0)).xyz;
	out_normal = (ubo_mvp.model * vec4(in_normal, 0.0)).xyz;
	out_texcoord2d = in_texcoord2d;
	out_tangent =  (ubo_mvp.model * vec4(in_tangent, 0.0)).xyz;
}
