in vec3 a_position;

out vec3 v_uv;

uniform mat4 u_mvpmatrix;

void main()
{
  vec4 v_position = vec4(a_position, 1.0);
  gl_Position = u_mvpmatrix * v_position;
  v_uv = vec3(a_position.x, a_position.y, a_position.z);
}
