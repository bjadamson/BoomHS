in vec3 a_position;

out vec3 v_uv;
out float v_visibility;

uniform mat4 u_mv;
uniform Fog u_fog;

void main()
{
  vec4 v_position = vec4(a_position, 1.0);
  gl_Position = u_mv * v_position;
  v_uv = vec3(a_position.x, a_position.y, a_position.z);
}
