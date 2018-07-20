in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_mvpmatrix;

out vec4 v_color;

void main()
{
  vec4 v_position = vec4(a_position, 1.0);
  gl_Position = u_mvpmatrix * v_position;

  v_color = vec4(0.0, 0.0, 0.0, 1.0);
}
