in vec3 a_position;

uniform mat4 u_mvpmatrix;
uniform vec3 u_lightcolor;

out vec4 v_color;

void main()
{
  gl_Position = u_mvpmatrix * vec4(a_position, 1.0);
  v_color = vec4(u_lightcolor, 1.0f);
}
