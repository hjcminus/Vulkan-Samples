#version 450

layout (binding = 1) uniform samplerCube texture_cube;

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec4 out_color;

void main() {
	vec3 uvw = normalize(in_pos);
	uvw.z *= -1.0;
	out_color = texture(texture_cube, uvw);
}
