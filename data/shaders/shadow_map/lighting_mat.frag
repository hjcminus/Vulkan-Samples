#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4  proj;
	mat4  view;
	mat4  model;
} ubo_mvp;

layout (binding = 2) uniform UBO_VIEWER {
	vec4  pos;
} ubo_viewer;

layout (binding = 3) uniform UBO_DIR_LIGHT {
    vec4  dir;
    vec4  color;
} ubo_dir_light;

layout (binding = 4) uniform sampler2D texture_depth;

layout (set = 1, binding = 0) uniform UBO_MATERIAL {
    vec4  ambient;
	vec4  diffuse;
	vec4  spedular;
	float shininess;
    float alpha;
} ubo_material;

layout (location = 0) in vec3 in_pos;		// in world space
layout (location = 1) in vec3 in_normal;	// in world space
layout (location = 2) in vec4 in_pos_light_space;
layout (location = 3) in float in_cos_theta;

layout (location = 0) out vec4 out_color;

const vec2 poisson_disk[4] = vec2[]
(
  vec2( -0.94201624,  -0.39906216 ),
  vec2(  0.94558609,  -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2(  0.34495938,   0.29387760 )
);

void main() {
	

	vec3 diff_color = ubo_material.diffuse.rgb;

	vec3 light_dir = -normalize(ubo_dir_light.dir.xyz);			// toward light
	vec3 view_dir  = normalize(ubo_viewer.pos.xyz - in_pos);	// toward viewer
	vec3 frag_normal = normalize(in_normal);

	float diff = max(dot(frag_normal, light_dir), 0.0);
	vec3 reflect_ = normalize(diff * frag_normal * 2.0 - light_dir);

	vec3 diffuse = diff_color * ubo_dir_light.color.rgb * diff;
	float spedular_factor = pow(max(0.0, dot(reflect_, view_dir)), ubo_material.shininess);
	vec3 spedular = ubo_material.spedular.rgb * ubo_dir_light.color.rgb * spedular_factor;

	// shadow

	vec2 uv = (in_pos_light_space.xy / in_pos_light_space.w + vec2(1.0, 1.0)) * 0.5;
	vec2 depth_tex_coord = vec2(uv.x, uv.y);
	
	float pixel_depth = in_pos_light_space.z / in_pos_light_space.w;	// vulkan [0-1]
	
	float bias = 0.005 * tan(acos(in_cos_theta)); // in_cos_theta is dot( n,l ), clamped between 0 and 1
    bias = clamp(bias, 0, 0.01);
    pixel_depth -= bias;

	float percent_in_light = 1.0;

	// PCF
    for (int i = 0; i < 4; ++i) {
        float tex_depth = texture(texture_depth, depth_tex_coord + poisson_disk[i] / 2000.0).x;
        if (tex_depth < pixel_depth) {
            percent_in_light -= 0.2;
        }
    }

	diffuse *= percent_in_light;

	out_color = vec4(ubo_material.ambient.rgb + diffuse + spedular, 1.0);
}
