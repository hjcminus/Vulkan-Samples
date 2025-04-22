#version 450

layout (binding = 0) uniform UBO_MVP {
    mat4 mvp;
} ubo_mvp;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_other;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out float out_cascade_idx;

void main() {
    gl_Position = ubo_mvp.mvp * vec4(in_pos, 1.0);
    out_uv = in_other.xy;
    out_cascade_idx = in_other.z;
}
