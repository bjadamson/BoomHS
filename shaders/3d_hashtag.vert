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
  const vec3 vertical_offsets[3] = vec3[3]( vec3(0.0, 0.0, 0.0), vec3(0.0, 0.1, 0.0), vec3(0.0, 0.2, 0.0) );
  const vec3 colors[3] = vec3[3]( vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0) );
  vec3 offset = vertical_offsets[gl_InstanceID] + u_offset;
  vec4 color = vec4(colors[gl_InstanceID], 1.0);

  vec4 pos = a_position + vec4(offset, 1.0);
  gl_Position = u_mvpmatrix * pos;

  v_normal = mat3(transpose(inverse(u_modelmatrix))) * a_normal;
  v_color = color;
  v_fragpos_worldspace = vec3(u_modelmatrix * a_position);
}
