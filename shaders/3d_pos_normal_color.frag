precision mediump float;

in vec4 v_position;
in vec3 v_surfacenormal;
in vec4 v_color;

in vec3 v_lightstofrag[MAX_NUM_POINTLIGHTS];

uniform Material    u_material;
uniform PointLight  u_pointlights[MAX_NUM_POINTLIGHTS];
uniform GlobalLight u_globallight;

uniform int   u_drawnormals;
uniform float u_reflectivity;

uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

out vec4 fragment_color;

void main()
{
  vec3 ambient = u_globallight.ambient * u_material.ambient;

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  vec3 dirlight = calc_dirlight(u_globallight, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, v_surfacenormal);

  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, v_surfacenormal);

  vec3 light = ambient + dirlight + pointlights;
  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  } else {
    fragment_color = vec4(light, 1.0) * v_color;
  }
}
