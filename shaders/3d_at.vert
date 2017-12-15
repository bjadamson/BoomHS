// attributes input to the vertex shader
in vec4 a_position; // position value
in vec4 a_color; // position value

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;

uniform mat4 u_mvmatrix;
uniform vec3 u_offset;

void main()
{
  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

  gl_Position = u_mvmatrix * a_position;
  v_color = color;
}
