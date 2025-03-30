#version 450

layout (binding = 0) uniform UBO_MVP {
    mat4 mvp;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;

void main() {
    gl_Position = ubo_mvp.mvp * vec4(in_pos, 1.0);
}
