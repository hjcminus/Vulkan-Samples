#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (std140, binding = 1) uniform UBO_POINT_LIGHT {
	vec4 pos;
	vec4 color;
	float radius;
} ubo_point_light;

layout (set = 1, binding = 0) uniform sampler2D texture_color;
layout (set = 1, binding = 1) uniform sampler2D normal_map;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord2d;
layout (location = 3) in vec3 in_tangent;

layout (location = 0) out vec4 out_frag_color;

const vec3 ambient = vec3(0.1, 0.1, 0.1);

void main() {
	vec3 N = normalize(in_normal);
	vec3 T = normalize(in_tangent);
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);

	vec3 frag_normal = texture(normal_map, in_texcoord2d).rgb;
	frag_normal = normalize(frag_normal * 2.0 - 1.0);	// convert to range [-1, 1]
	frag_normal = TBN * frag_normal;

	vec4 mat_color = texture(texture_color, in_texcoord2d);

	vec3 light_dir = normalize(ubo_point_light.pos.xyz - in_pos);

	float diff = max(dot(frag_normal, light_dir), 0.0);
	
	vec3 diffuse = ubo_point_light.color.rgb * diff;
	vec3 combined = ambient * mat_color.rgb + diffuse * mat_color.rgb;

	out_frag_color = vec4(combined, 1.0);
}
