#version 300 es

// attributes input to the vertex shader
layout (location = 0) in vec4 a_position; // position value
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;
out vec2 v_uv;

void main()
{
  gl_Position = a_position;
  v_color = a_color;
  v_uv = a_uv;
}
