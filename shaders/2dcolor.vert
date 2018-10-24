in vec3 a_position;

uniform mat4 u_mv;
uniform vec4 u_color;

out vec4 v_color;

void main()
{
  gl_Position = u_mv * vec4(a_position, 1.0);
  v_color = u_color;
}
