#version 450

layout (binding = 0) uniform UBO_VP {
    mat4 vp;
} ubo_vp;

layout (binding = 1) uniform UBO_MODEL {
	mat4 matrix;
} ubo_model;

// bind 0
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

// bind 1
layout (location = 2) in vec3 in_inst_pos;
layout (location = 3) in vec3 in_inst_scale;


layout (location = 0) out vec2 out_uv;

void main() {
	vec3 new_pos;
	
	new_pos.x = in_pos.x * in_inst_scale.x;
	new_pos.y = in_pos.y * in_inst_scale.y;
	new_pos.z = in_pos.z * in_inst_scale.z;

	new_pos = (ubo_model.matrix * vec4(new_pos, 1.0)).xyz;

	new_pos += in_inst_pos;

    gl_Position = ubo_vp.vp * vec4(new_pos, 1.0);
	
	out_uv = in_uv;
}


