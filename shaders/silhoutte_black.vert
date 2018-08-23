in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_mvpmatrix;

void main()
{
  gl_Position = u_mvpmatrix * vec4(a_position, 1.0);
}
