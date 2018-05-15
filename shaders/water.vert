in vec3 a_position;
in vec3 a_normal;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_clipspace;
out float v_visibility;
out float v_clipdistance;

uniform Fog u_fog;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;

uniform mat4 u_mvpmatrix;
uniform mat3 u_normalmatrix;
uniform vec4 u_clipPlane;

void main()
{
  v_position = vec4(a_position, 1.0);
  v_clipspace = u_mvpmatrix * v_position;
  gl_Position = v_clipspace;

  v_surfacenormal = normalize(u_normalmatrix * a_normal);

  v_visibility = calculate_fog_visibility(u_fog, u_modelmatrix, u_viewmatrix, v_position);

  vec4 model_pos = u_modelmatrix * vec4(a_position, 1.0);
  v_clipdistance = dot(model_pos, u_clipPlane);
}
