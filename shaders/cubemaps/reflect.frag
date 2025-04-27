#version 450

layout (binding = 1) uniform samplerCube texture_cube;

layout (binding = 2) uniform UBO_POINT_LIGHT {
	vec4 pos;
	vec4 color;
	float radius;
} ubo_light;

layout (binding = 3) uniform UBO_VIEWER {
	vec4  pos;
} ubo_viewer;

layout (location = 0) in vec3 in_pos;				// in world space
layout (location = 1) in vec3 in_normal_in_local;
layout (location = 2) in vec3 in_normal_in_world;

layout (location = 0) out vec4 out_color;

const float SHINESS  = 8.0;

void main() {
	vec3 uvw = normalize(in_normal_in_local);
	uvw.z *= -1.0;

	vec3 base_color = texture(texture_cube, uvw).xyz;

	vec3 light_dir = normalize(ubo_light.pos.xyz - in_pos);	// toward the light
	vec3 view_dir = normalize(ubo_viewer.pos.xyz - in_pos);			// toward the viewer
	vec3 frag_normal = normalize(in_normal_in_world);

	float diff = max(dot(frag_normal, light_dir), 0.0);
	vec3 diff_color = base_color * ubo_light.color.xyz * diff;

	vec3 reflect_ = reflect(-light_dir, frag_normal);
	float spedular_factor = pow(max(0.0, dot(reflect_, view_dir)), SHINESS);
	vec3 spedular_color = ubo_light.color.xyz * spedular_factor;

	out_color = vec4(diff_color + spedular_color, 1.0);
}
