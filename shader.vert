#version 300 es

// attributes input to the vertex shader
in vec4 a_position; // position value

// output of the vertex shader - input to fragment
// shader
out vec4 out_position;

void main()
{
  // for now, no transformations.
  gl_Position = a_position;
}
