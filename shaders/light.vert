in vec4 a_position;

uniform mat4 u_mvpmatrix;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
}
