
const vec2 poisson_disk[4] = vec2[]
(
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760)
);

const vec3 debug_color_table[4] = vec3[]
(
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 1.0, 0.0)
);


vec3 debug_color = vec3(0.0);

float calc_percent_in_light(vec3 normal_in_world, vec3 light_dir) {
	float percent_in_light = 1.0;

	for (int i = 0; i < SHADOW_MAP_COUNT; ++i) {
		if (gl_FragCoord.z <= ubo_cas.z_far[i].x) {

		    debug_color = debug_color_table[i];

		    ivec2 depth_tex_sz = textureSize(texture_depth, i).xy;

			vec4 pos_in_light_space = ubo_cas.light_vp[i] * ubo_cas.inv_view_mvp * in_pos;

			vec2 depth_tex_coord = (pos_in_light_space.xy / pos_in_light_space.w + vec2(1.0, 1.0)) * 0.5;	// convert to [0.0-1.0]
			
			float pixel_depth = pos_in_light_space.z / pos_in_light_space.w;	// vulkan [0-1]
			float bias = 0.005;
			pixel_depth -= bias;

			// PCF
			for (int j = 0; j < 4; ++j) {
				float tex_depth = texture(texture_depth, vec3((depth_tex_coord + poisson_disk[j] / 2000.0), i)).x;
				if (tex_depth < pixel_depth) {
					percent_in_light -= 0.2;
				}
			}

			break;
		}
	}

	return percent_in_light;
}