in vec3 a_position;

uniform mat4 u_mv;

void main()
{
  gl_Position = u_mv * vec4(a_position, 1.0);
}
