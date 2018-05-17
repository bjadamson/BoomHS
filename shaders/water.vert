in vec3 a_position;
in vec2 a_textureuv;

out vec4 v_position;
out vec2 v_textureuv;
out vec4 v_clipspace;
out float v_visibility;
out float v_clipdistance;
out vec2 v_fbouv;
out vec3 v_tocamera;

uniform Fog u_fog;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;

uniform mat4 u_mvpmatrix;
uniform vec4 u_clipPlane;
uniform vec3 u_camera_position;

const float TILING = 6.0;

void main()
{
  v_position = vec4(a_position, 1.0);
  v_clipspace = u_mvpmatrix * v_position;
  gl_Position = v_clipspace;
  v_textureuv = a_textureuv;

  v_visibility = calculate_fog_visibility(u_fog, u_modelmatrix, u_viewmatrix, v_position);

  vec4 model_pos = u_modelmatrix * vec4(a_position, 1.0);
  v_clipdistance = dot(model_pos, u_clipPlane);

  v_fbouv = vec2(v_position.xy/2.0 + 0.5) * TILING;
  v_tocamera = u_camera_position - model_pos.xyz;
}
