in vec4 a_position;
in vec4 a_color;

uniform mat4 u_mvpmatrix;

out vec4 v_color;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
  v_color = a_color;
}
