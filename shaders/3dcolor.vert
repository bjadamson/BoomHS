// attributes input to the vertex shader
in vec4 a_position; // position value
in vec4 a_color;

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_mvmatrix;

void main()
{
  gl_Position = u_mvmatrix * u_projection * u_view * a_position;
  v_color = a_color;
}
