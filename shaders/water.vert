in vec3 a_position;
in vec3 a_normal;
in vec2 a_diffuseuv;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec2 v_diffuseuv;
out vec4 v_clipspace;
out float v_visibility;
out float v_clipdistance;
out vec2 v_dudv;
out vec3 v_tocamera;

uniform Fog u_fog;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;

uniform mat4 u_mvpmatrix;
uniform mat3 u_normalmatrix;
uniform vec4 u_clipPlane;
uniform vec3 u_camera_position;

const float TILING = 6.0;

void main()
{
  v_position = vec4(a_position, 1.0);
  v_clipspace = u_mvpmatrix * v_position;
  gl_Position = v_clipspace;
  v_diffuseuv = a_diffuseuv;

  v_surfacenormal = normalize(u_normalmatrix * a_normal);

  v_visibility = calculate_fog_visibility(u_fog, u_modelmatrix, u_viewmatrix, v_position);

  vec4 model_pos = u_modelmatrix * vec4(a_position, 1.0);
  v_clipdistance = dot(model_pos, u_clipPlane);

  v_dudv = vec2(v_position.xy/2.0 + 0.5) * TILING;
  v_tocamera = u_camera_position - model_pos.xyz;
}
