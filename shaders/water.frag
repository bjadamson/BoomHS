in vec4 v_position;
in vec3 v_surfacenormal;
in float v_visibility;
in float v_clipdistance;

uniform sampler2D u_reflect_sampler;
uniform sampler2D u_refract_sampler;

uniform Material         u_material;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform AmbientLight     u_ambient;
uniform DirectionalLight u_directional_light;

uniform int   u_drawnormals;
uniform int   u_ignore_dirlight;
uniform float u_reflectivity;
uniform mat4 u_mvpmatrix;

uniform Fog u_fog;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

out vec4 fragment_color;

void main()
{
  clip_check(v_clipdistance);

  vec3 ambient = u_ambient.color * u_material.ambient;

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  vec3 dirlight = calc_dirlight(u_directional_light, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, v_surfacenormal);

  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, v_surfacenormal);

  vec3 ambient_pointlight = ambient + pointlights;
  vec3 light = ambient_pointlight + dirlight;
  vec4 light_color = vec4(light, 1.0);

  if (u_drawnormals == 1) {
    light_color = vec4(v_surfacenormal, 1.0);
  }
  else if (u_ignore_dirlight == 1) {
    light_color = vec4(ambient_pointlight, 1.0) * light_color;
  }
  else {
    light_color = vec4(light, 1.0) * light_color;
  }
  light_color = mix(u_fog.color, light_color, v_visibility);

  vec4 clipspace = u_mvpmatrix * v_position;
  vec2 ndc         = ((clipspace.xy/clipspace.w)/2.0) + 0.5;
  vec2 reflect_uv  = vec2(ndc.x, -ndc.y);
  vec2 refract_uv  = vec2(ndc.x, ndc.y);

  vec4 reflect     = texture(u_reflect_sampler, reflect_uv);
  vec4 refract     = texture(u_refract_sampler, refract_uv);
  vec4 texture_mix = mix(reflect, refract, 0.5);

  // The output color is a mixture of the blended textures and the calculated light.
  fragment_color = mix(texture_mix, light_color, 0.1);

  const vec4 BLUE_MIX = vec4(0.0, 0.1, 0.6, 1.0);
  fragment_color = mix(fragment_color, BLUE_MIX, 0.4);
}
