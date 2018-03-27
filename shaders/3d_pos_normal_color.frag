precision mediump float;

in vec4 v_position;
in vec3 v_surfacenormal;
in vec4 v_color;

uniform Material         u_material;
uniform AmbientLight     u_ambient;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform DirectionalLight u_directional_light[MAX_NUM_DIRECTIONAL_LIGHTS];

uniform int   u_drawnormals;
uniform int   u_ignore_dirlight;
uniform float u_reflectivity;

uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

out vec4 fragment_color;

void main()
{
  vec3 ambient = u_ambient.color * u_material.ambient;

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  vec3 dirlight = calc_dirlight(u_directional_light, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, v_surfacenormal);

  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, v_surfacenormal);

  vec3 ambient_pointlight = ambient + pointlights;
  vec3 light = ambient_pointlight + dirlight;
  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  } else if (u_ignore_dirlight == 1) {
    fragment_color = vec4(ambient_pointlight, 1.0) * v_color;
  }
  else {
    fragment_color = vec4(light, 1.0) * v_color;
  }
}
