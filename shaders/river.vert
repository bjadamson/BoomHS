in vec4 a_position;
in vec3 a_normal;

out vec4 v_position;
out vec3 v_surfacenormal;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat3 u_normalmatrix;

void main()
{
  const float offset_z = 0.15;
  const vec3 vertical_offsets[2] = vec3[2]( vec3(0.0, 0.0, -offset_z), vec3(0.0, 0.0, offset_z) );
  vec3 offset = vertical_offsets[gl_InstanceID];

  v_position = a_position;
  v_surfacenormal = normalize(u_normalmatrix * a_normal);

  // Transform v_position to world-space
  vec4 pos_world = u_modelmatrix * v_position;

  // Calculate the position of the vertex using the offset array
  pos_world += vec4(offset, 0.0);

  // Transform pos_world to it's original space
  pos_world = inverse(u_modelmatrix) * pos_world;

  // Transform pos_world to ndc
  gl_Position = u_mvpmatrix * pos_world;
}
