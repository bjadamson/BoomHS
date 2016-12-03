// attributes input to the vertex shader
in vec4 a_position; // position value
in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec2 v_uv;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
  gl_Position = u_projection * u_view * a_position;
  v_uv = a_uv;
}
