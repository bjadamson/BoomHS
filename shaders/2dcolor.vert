in vec3 a_position;
in vec4 a_color;

out vec4 v_color;

uniform mat4 u_modelmatrix;

void main()
{
  gl_Position = u_modelmatrix * vec4(a_position, 1.0);
  v_color = a_color;
}
