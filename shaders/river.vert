in vec4 a_position;
in vec3 a_normal;

out vec4 v_position;
out vec3 v_surfacenormal;

uniform vec2 u_offset;
uniform vec2 u_direction;
uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat3 u_normalmatrix;

void main()
{
  const float offset_z = 0.10;
  const float offset_x = 0.25f;

  vec3 direction = vec3(u_direction.x, 0.0, u_direction.y);
  vec3 right = normalize(cross(direction, vec3(0, 1, 0)));

  vec3 a = (direction * -offset_z) + (right * offset_x);
  vec3 b = (direction * offset_z) + (right * offset_x);

  vec3 c = (direction * -offset_z) + (right * -offset_x);
  vec3 d = (direction * offset_z) + (right * -offset_x);

  vec3 offsets[4] = vec3[4](a, b, c, d);
  vec3 offset = offsets[gl_InstanceID];

  v_position = a_position;
  v_surfacenormal = normalize(u_normalmatrix * a_normal);

  // Transform v_position to world-space
  vec4 pos_world = u_modelmatrix * v_position;
  pos_world += vec4(u_offset.x, pos_world.y, u_offset.y, 0.0);

  // Calculate the position of the vertex using the offset array
  pos_world += vec4(offset, 0.0);

  // Transform pos_world to it's original space
  pos_world = inverse(u_modelmatrix) * pos_world;

  // Transform pos_world to ndc
  gl_Position = u_mvpmatrix * pos_world;
}
