#version 450

layout (binding = 1) uniform sampler2DArray texture_depth;

layout (location = 0) in vec2 in_uv;
layout (location = 1) in flat float in_cascade_idx;

layout (location = 0) out vec4 out_color;

float LinearizeDepth(float depth) {
    float n = 2.0;
    float f = 32.0;
    float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

void main() {
	float depth = texture(texture_depth, vec3(in_uv.x, in_uv.y, in_cascade_idx)).x;
	float v = LinearizeDepth(depth);
	out_color = vec4(v, v, v, 1.0);
}
