in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

// LIGHTING
uniform mat4 u_mvpmatrix;
uniform mat3 u_normalmatrix;

// FOG
uniform Fog u_fog;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;
out float v_visibility;

void main()
{
  v_position = a_position;
  gl_Position = u_mvpmatrix * v_position;

  v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_color = a_color;

  v_visibility = calculate_fog_visibility(u_fog, u_modelmatrix, u_viewmatrix, v_position);
}
