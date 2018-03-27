in vec4 a_position;
in vec2 a_uv;

uniform mat4 u_mvpmatrix;
uniform vec3 u_lightcolor;

out vec4 v_color;
out vec2 v_uv;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
  v_color = vec4(u_lightcolor, 1.0f);
  v_uv = a_uv;
}
