#version 450

layout (set = 1, binding = 0) uniform sampler2D texture_color;

layout (location = 0) in vec2 in_texcoord2d;

layout (location = 0) out vec4 out_frag_color;

void main() {
	out_frag_color = texture(texture_color, in_texcoord2d);
}
