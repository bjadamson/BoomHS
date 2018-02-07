in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;

uniform float u_ticks;
uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat3 u_normalmatrix;

void main()
{
  const float offset_z = 0.15;
  const vec3 vertical_offsets[2] = vec3[2]( vec3(0.0, 0.0, -offset_z), vec3(0.0, 0.0, offset_z) );
  float zoffset = mix(0, sin(u_ticks) / 50.0, 1);
  vec3 offset = vertical_offsets[gl_InstanceID] + (vec3(0.0, 0.0, zoffset));

  v_position = a_position;
  v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_color = a_color;

  // Transform v_position to world-space
  vec4 pos_world = u_modelmatrix * v_position;

  // Calculate the position of the vertex using the offset array
  pos_world += vec4(offset, 0.0);

  // Transform pos_world to it's original space
  pos_world = inverse(u_modelmatrix) * pos_world;

  // Transform pos_world to ndc
  gl_Position = u_mvpmatrix * pos_world;
}
