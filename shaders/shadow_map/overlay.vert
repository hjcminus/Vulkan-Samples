#version 450

layout (binding = 0) uniform UBO_MVP {
    mat4 mvp;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

void main() {
    gl_Position = ubo_mvp.mvp * vec4(in_pos, 1.0);
    out_uv = in_uv;
}
