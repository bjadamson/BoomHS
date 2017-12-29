// attributes input to the vertex shader
in vec4 a_position; // position value
in vec4 a_color;

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;

uniform mat4 u_mvpmatrix;
uniform vec3 u_offset;

void main()
{
  vec4 pos = a_position + vec4(u_offset, 1.0);
  pos -= vec4(0.0, 0.5, 0.0, 0.0);
  gl_Position = u_mvpmatrix * pos;
  v_color = a_color;
}
