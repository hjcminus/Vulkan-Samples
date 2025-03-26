#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (binding = 1) uniform UBO_VIEWER {
	vec4  pos;
} ubo_viewer;

layout (binding = 2) uniform UBO_DIR_LIGHT {
	vec4 dir;
	vec4 color;
} ubo_dir_light;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;

layout (location = 0) out vec4 out_color;

const float SHINESS  = 32.0;
const vec3  AMBIENT  = vec3(0.1, 0.1, 0.1);
const vec3  SPEDULAR = vec3(1.0, 1.0, 1.0);

void main() {
	vec3 diff_color = in_color;

	vec3 light_dir = -normalize(ubo_dir_light.dir.xyz);			// toward light
	vec3 view_dir  = normalize(ubo_viewer.pos.xyz - in_pos);	// toward viewer
	vec3 frag_normal = normalize(in_normal);

	float diff = max(dot(frag_normal, light_dir), 0.0);
	vec3 reflect_ = normalize(diff * frag_normal * 2.0 - light_dir);

	vec3 diffuse = diff_color * ubo_dir_light.color.rgb * diff;
	float spedular_factor = pow(max(0.0, dot(reflect_, view_dir)), SHINESS);
	vec3 spedular = SPEDULAR * ubo_dir_light.color.rgb * spedular_factor;

	out_color = vec4(AMBIENT + diffuse + spedular, 1.0);
}

