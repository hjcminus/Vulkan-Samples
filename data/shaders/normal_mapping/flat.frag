#version 450

layout (std140, binding = 1) uniform UBO_LIGHT {
	vec4 pos;
	vec4 color;
	float radius;
} ubo_light;

layout (set = 1, binding = 0) uniform sampler2D texture_color;


layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord2d;

layout (location = 0) out vec4 out_frag_color;

const vec3 ambient = vec3(0.1, 0.1, 0.1);

void main() {
	vec4 mat_color = texture(texture_color, in_texcoord2d);

	vec3 light_dir = normalize(ubo_light.pos.xyz - in_pos);
	vec3 frag_normal = normalize(in_normal);

	float diff = max(dot(frag_normal, light_dir), 0.0);
	
	vec3 diffuse = ubo_light.color.rgb * diff;
	vec3 combined = ambient * mat_color.rgb + diffuse * mat_color.rgb;

	out_frag_color = vec4(combined, 1.0);
}
