#version 450

layout (binding = 0) uniform UBO_MVP {
	mat4  proj;
	mat4  view;
	mat4  model;
} ubo_mvp;

layout (set = 3, binding = 0) uniform UBO_TERRAIN {
	float edge_length;
} ubo_terrain;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec4 out_pos;
layout (location = 1) out vec3 out_pos_in_view;
layout (location = 2) out vec3 out_pos_in_world;
layout (location = 3) out vec3 out_normal_in_world;
layout (location = 4) out vec2 out_uv1;
layout (location = 5) out vec2 out_uv2;

void main() {
	vec4 pos_in_world = ubo_mvp.model * vec4(in_pos, 1.0);
	vec4 pos_in_view = ubo_mvp.view * pos_in_world;

	out_pos = ubo_mvp.proj * pos_in_view;
	gl_Position = out_pos;

	out_pos_in_view = pos_in_view.xyz;
	out_pos_in_world = pos_in_world.xyz;
	out_normal_in_world = (ubo_mvp.model * vec4(in_normal, 0.0)).xyz;

	float inv = 1.0 / ubo_terrain.edge_length;

	// base texture
	out_uv1.x = in_pos.x * inv;
	out_uv1.y = in_pos.y * inv;

	// detail texture
	out_uv2.x = in_pos.x * 0.25;
	out_uv2.y = in_pos.y * 0.25;
}
