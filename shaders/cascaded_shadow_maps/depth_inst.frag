#version 450

layout (set = 1, binding = 1) uniform sampler2D texture_color;

layout (location = 0) in vec2 in_uv;

void main() {
	vec4 color = texture(texture_color, in_uv);
	if (color.a < 0.66) {
		discard;
	}
}
