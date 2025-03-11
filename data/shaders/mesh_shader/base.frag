#version 450
 
layout (location = 0) in VertexInput {
  vec4 color;
} vertex_input;

layout(location = 0) out vec4 out_frag_color;
 
void main() {
	out_frag_color = vertex_input.color;
}