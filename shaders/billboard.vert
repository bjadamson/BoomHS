in vec3 a_position;
in vec2 a_uv;

uniform mat4 u_mv;

out vec2 v_uv;

void main()
{
  gl_Position = u_mv * vec4(a_position, 1.0);
  v_uv = a_uv;
}
