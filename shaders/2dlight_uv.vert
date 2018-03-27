in vec4 a_position;
in vec2 a_uv;

uniform mat4 u_modelmatrix;
uniform vec3 u_lightcolor;

out vec2 v_uv;

void main()
{
  gl_Position = u_modelmatrix * a_position;
  v_uv = a_uv;
}
