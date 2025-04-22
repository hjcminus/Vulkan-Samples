#version 450

layout (binding = 0) uniform UBO_VP {
    mat4 vp;
} ubo_vp;

layout (location = 0) in vec3 in_pos;

void main() {
    gl_Position = ubo_vp.vp * vec4(in_pos, 1.0);
}
