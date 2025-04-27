#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec3 out_pos;

void main() {
    gl_Position = ubo_mvp.proj * ubo_mvp.view * ubo_mvp.model * vec4(in_pos, 1.0);
    out_pos = in_pos;
}
