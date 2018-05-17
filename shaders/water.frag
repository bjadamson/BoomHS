in vec4 v_position;
in vec3 v_surfacenormal;
in vec2 v_diffuseuv;
in float v_visibility;
in float v_clipdistance;
in vec4 v_clipspace;
in vec2 v_dudv;
in vec3 v_tocamera;

uniform sampler2D u_texture_sampler;
uniform sampler2D u_reflect_sampler;
uniform sampler2D u_refract_sampler;
uniform sampler2D u_dudv_sampler;

uniform Material         u_material;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform AmbientLight     u_ambient;
uniform DirectionalLight u_directional_light;

uniform int   u_drawnormals;
uniform int   u_ignore_dirlight;
uniform float u_reflectivity;

uniform Fog u_fog;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

uniform float u_dudv_offset;

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

  vec2 ndc         = ((v_clipspace.xy/v_clipspace.w)/2.0) + 0.5;
  ndc.y = 1.0 - ndc.y;
  vec2 reflect_uv  = vec2(ndc.x, -1.0 +ndc.y);
  vec2 refract_uv  = vec2(ndc.x, 1.0 - ndc.y);

  vec2 distortion1 = texture(u_dudv_sampler, vec2(v_dudv.x + u_dudv_offset, v_dudv.y)).rg * 2.0 - 1.0;
  vec2 distortion2 = texture(u_dudv_sampler, vec2(-v_dudv.x + u_dudv_offset, v_dudv.y + u_dudv_offset)).rg * 2.0 - 1.0;
  vec2 distortion = distortion1 + distortion2;

  const float WAVE_STRENGTH = 0.005;
  reflect_uv += distortion * WAVE_STRENGTH;
  refract_uv += distortion * WAVE_STRENGTH;

  const float CLAMP         = 0.001;
  const float INVERSE_CLAMP = (1.0 - CLAMP);

  refract_uv = clamp(refract_uv, CLAMP, INVERSE_CLAMP);

  reflect_uv.x = clamp(reflect_uv.x, CLAMP, INVERSE_CLAMP);
  reflect_uv.y = clamp(reflect_uv.y, -INVERSE_CLAMP, -CLAMP);

  vec4 reflect_color = texture(u_reflect_sampler, reflect_uv);
  vec4 refract_color = texture(u_refract_sampler, refract_uv);

  vec3 view_vector = normalize(v_tocamera);
  float refractive_factor = dot(view_vector, vec3(0.0, 1.0, 0.0));

  const float FRESNEL_REFLECTIVE_FACTOR = 2.0;
  refractive_factor = pow(refractive_factor, FRESNEL_REFLECTIVE_FACTOR);

  const float weight_light   = 0.8;
  const float weight_texture = 0.5;
  const float weight_effects = 1.0;

  vec4 effect_color  = mix(reflect_color, refract_color, refractive_factor) * weight_effects;
  vec4 texture_color = texture(u_texture_sampler, v_diffuseuv) * weight_texture;
  light_color        = light_color * weight_light;

  fragment_color = texture_color + effect_color + texture_color + light_color;

  const vec4 BLUE_MIX = vec4(0.0, 0.3, 0.8, 1.0);
  fragment_color = mix(fragment_color, BLUE_MIX, 0.6);
}
