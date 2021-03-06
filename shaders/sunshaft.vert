in vec3 a_position;
in vec2 a_uv;

uniform mat4 u_modelmatrix;

out vec2 v_uv;

void main()
{
  gl_Position = u_modelmatrix * vec4(a_position, 1.0);
  v_uv = vec2(a_uv.x, 1.0 - a_uv.y);
}
