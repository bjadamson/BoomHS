#version 300 es

// attributes input to the vertex shader
layout (location = 0) in vec4 a_position; // position value
layout (location = 1) in vec4 a_color;

// output of the vertex shader - input to fragment
// shader
out vec4 vertex_color;

void main()
{
  gl_Position = a_position;
  vertex_color = a_color;
}
