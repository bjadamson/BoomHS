// attributes input to the vertex shader
in vec4 a_position; // position value
in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec2 v_uv;

void main()
{
  gl_Position = a_position;
  v_uv = a_uv;
}
