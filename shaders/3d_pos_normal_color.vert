in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec3 v_normal;
out vec4 v_color;
out vec3 v_fragpos_worldspace;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;

void main()
{
  gl_Position = u_mvpmatrix * a_position;

  v_normal = a_normal;
  v_color = a_color;
  v_fragpos_worldspace = vec3(u_modelmatrix * a_position);
}
