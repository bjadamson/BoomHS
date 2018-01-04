in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec3 v_normal;
out vec4 v_color;
out vec3 v_fragpos_worldspace;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform vec3 u_offset;

void main()
{
  vec4 pos = a_position + vec4(u_offset, 1.0);
  pos -= vec4(0.0, 1.0, 0.0, 0.0);
  gl_Position = u_mvpmatrix * pos;

  v_normal = mat3(transpose(inverse(u_modelmatrix))) * a_normal;
  v_color = a_color;
  v_fragpos_worldspace = vec3(u_modelmatrix * a_position);
}
