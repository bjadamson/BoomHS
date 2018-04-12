in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;
out float v_visibility;

// LIGHTING
uniform mat4 u_mvpmatrix;
uniform mat4 u_inversemodelmatrix;
uniform mat3 u_normalmatrix;

// FOG
uniform Fog u_fog;
uniform mat4 u_viewmatrix;

// SHARED
uniform mat4 u_modelmatrix;

void main()
{
  const float offset_y = 0.5;
  const vec3 vertical_offsets[3] = vec3[3](
      vec3(0.0, 0.0, 0.0),
      vec3(0.0, offset_y, 0.0),
      vec3(0.0, 2.0 * offset_y, 0.0)
      );
  vec3 offset = vertical_offsets[gl_InstanceID];
  vec4 color = v_color;

  v_position = vec4(a_position, 1.0);
  v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_color = a_color;

  // Transform v_position to world-space
  vec4 pos_world = u_modelmatrix * v_position;

  // Calculate the position of the vertex using the offset array
  pos_world += vec4(offset, 0.0);

  // Transform pos_world to it's original space
  pos_world = u_inversemodelmatrix * pos_world;

  // Transform pos_world to ndc
  gl_Position = u_mvpmatrix * pos_world;

  v_visibility = calculate_fog_visibility(u_fog, u_modelmatrix, u_viewmatrix, v_position);
}
