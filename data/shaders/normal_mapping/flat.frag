#version 450

layout (std140, binding = 1) uniform UBO_LIGHT {
	vec4 light_pos;
	vec4 light_color;
} ubo_light;

layout (set = 1, binding = 0) uniform sampler2D texture_color;


layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord2d;

layout (location = 0) out vec4 out_frag_color;

const vec3 ambient = vec3(0.1f, 0.1f, 0.1f);

void main() {
	vec4 mat_color = texture(texture_color, in_texcoord2d);

	vec3 light_dir = normalize(ubo_light.light_pos.xyz - in_pos);
	vec3 frag_normal = normalize(in_normal);

	float diff = max(dot(frag_normal, light_dir), 0.0f);
	
	vec3 diffuse = ubo_light.light_color.rgb * diff;
	vec3 combined = ambient * mat_color.rgb + diffuse * mat_color.rgb;

	out_frag_color = vec4(combined, 1.0f);
}
