in vec4 v_position;
in vec3 v_surfacenormal;
in float v_visibility;

uniform Material    u_material;
uniform PointLight  u_pointlights[MAX_NUM_POINTLIGHTS];
uniform DirectionalLight u_directional_light;

uniform int   u_drawnormals;
uniform int   u_ignore_dirlight;
uniform float u_reflectivity;

uniform Fog u_fog;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;
uniform vec4 u_color;

out vec4 fragment_color;

void main()
{
  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, v_surfacenormal);

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  vec3 dirlight = calc_dirlight(u_directional_light, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, v_surfacenormal);

  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  }
  else if (u_ignore_dirlight == 1) {
    fragment_color = vec4(pointlights, 1.0) * u_color;
  }
  else {
    fragment_color = vec4(dirlight + pointlights, 1.0) * u_color;
  }
  fragment_color = mix(u_fog.color, fragment_color, v_visibility);
}
