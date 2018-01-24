in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat3 u_normalmatrix;

uniform vec3 u_offset;

void main()
{
  v_position = a_position;
  v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_color = vec4(0.0, 1.0, 0.0, 1.0);

  // Transform v_position to world-space
  vec4 pos_world = u_modelmatrix * v_position;

  // Move the point down in the Y axis
  pos_world += vec4(u_offset, 0.0);

  // Transform pos_world to it's original space
  pos_world = inverse(u_modelmatrix) * pos_world;

  // Transform pos_world to ndc
  gl_Position = u_mvpmatrix * pos_world;
}
