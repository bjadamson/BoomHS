in vec4 a_position;

uniform mat4 u_mvpmatrix;
uniform vec3 u_lightcolor;

out vec4 v_color;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
  v_color = vec4(u_lightcolor, 1.0f);
}
