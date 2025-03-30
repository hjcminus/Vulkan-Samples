#version 450

layout (binding = 4) uniform sampler2D texture_depth;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

float LinearizeDepth(float depth) {
    float n = 2.0;
    float f = 32.0;
    float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

void main() {
	//float depth = texture(texture_depth, vec2(in_uv.x, 1.0-in_uv.y)).x;
	float depth = texture(texture_depth, vec2(in_uv.x, in_uv.y)).x;
	//float v = LinearizeDepth(depth);
	float v = depth;
	out_color = vec4(v, v, v, 1.0);
}
