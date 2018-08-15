in vec3 a_position;

uniform mat4 u_invviewproj;

void main()
{
  gl_Position = u_invviewproj * vec4(a_position, 1.0);
  gl_Position = gl_Position / gl_Position.w;
}
