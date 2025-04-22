#version 450

// #extension GL_EXT_scalar_block_layout : enable
// #extension GL_ARB_shading_language_include : require

#define	SHADOW_MAP_COUNT	4

layout (binding = 0) uniform UBO_MVP {
	mat4 proj;
	mat4 view;
	mat4 model;
} ubo_mvp;

layout (binding = 1) uniform UBO_VIEWER {
	vec4  pos;
} ubo_viewer;

layout (binding = 2) uniform sampler2DArray texture_depth;

layout (set = 1, binding = 0) uniform sampler2D texture_terrain;
layout (set = 1, binding = 1) uniform sampler2D texture_detail;

layout (set = 2, binding = 0) uniform UBO_DIR_LIGHT {
    vec4  dir;
    vec4  color;
} ubo_dir_light;

layout (set = 4, binding = 0) uniform UBO_FOG {
	vec4 params;	// x: start, y: end, z: scale, w: density
	vec4 color;
} ubo_fog;

layout (set = 5, binding = 0) uniform UBO_CASCADED_SHADOW_MAPS {
	mat4 inv_view_mvp;
	vec4 z_far[SHADOW_MAP_COUNT];
	mat4 light_vp[SHADOW_MAP_COUNT];
} ubo_cas;

layout(push_constant) uniform PUSH_CONSTANT {
	uvec4 options;	// x: draw_debug, y: draw_fog
} push_const;

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec3 in_pos_in_view;
layout (location = 2) in vec3 in_pos_in_world;
layout (location = 3) in vec3 in_normal_in_world;
layout (location = 4) in vec2 in_uv1;
layout (location = 5) in vec2 in_uv2;

layout (location = 0) out vec4 out_color;

#include "shared.glsl"


void main() {
    uint draw_debug = push_const.options.x;
	uint draw_fog = push_const.options.y;

	vec4 color_base = texture(texture_terrain, in_uv1);
    vec4 color_detail = texture(texture_detail, in_uv2);

	// mix(x, y, a):  x * (1-a) + y * a
	vec3 terrain_color = mix(color_base, color_detail, 0.4).rgb;

	vec3 light_dir = -normalize(ubo_dir_light.dir.xyz);
	vec3 normal_in_world = normalize(in_normal_in_world);
	float diff = max(dot(normal_in_world, light_dir), 0.0);

	vec3 diffuse = terrain_color * ubo_dir_light.color.rgb * diff;
	float percent_in_light = calc_percent_in_light(normal_in_world, light_dir);

	if (draw_debug == 1) {
		out_color = vec4(debug_color + diffuse * percent_in_light, 1.0);
	}
	else {
		vec3 non_fog_color = diffuse * percent_in_light;

		if (draw_fog == 1) {
			float fog_end = ubo_fog.params.y;
			float fog_scale = ubo_fog.params.z;
			float fog_density = ubo_fog.params.w;

			// note: fog_end - (-in_pos_in_view.z) == fog_end + in_pos_in_view.z
			float fog = clamp((fog_end + in_pos_in_view.z) * fog_scale * fog_density, 0.0, 1.0);

			out_color = vec4(mix(ubo_fog.color.rgb, non_fog_color, fog), 1.0);
		}
		else {
			out_color = vec4(non_fog_color, 1.0);
		}
	}
}
