in vec4 a_position;

uniform mat4 u_mvpmatrix;

out vec3 v_uv;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
  v_uv = vec3(a_position.x, a_position.y, a_position.z);
}
