in vec4 a_position;

uniform mat4 u_mvmatrix;

void main()
{
  gl_Position = u_mvmatrix * a_position;
}
