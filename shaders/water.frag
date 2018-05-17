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
uniform sampler2D u_normal_sampler;

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

// water related
uniform float u_dudv_offset;
uniform float u_wavestrength;
uniform float u_diffuse_offset;

out vec4 fragment_color;

struct DirlightColors
{
  vec3 diffuse;
  vec3 specular;
};

DirlightColors
calc_dirlight_special(DirectionalLight dir_light, Material material, float reflectivity,
    vec3 frag_world_pos, mat4 invviewmatrix, vec3 surface_normal)
{
  vec3 light_dir = dir_light.direction;
  float diff = max(dot(surface_normal, light_dir), 0.0);
  vec3 diffuse = diff * dir_light.diffuse;

  vec3 camera_position = (invviewmatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  vec3 v_cameratofrag = normalize(camera_position - frag_world_pos);

  vec3 reflected_light_v = reflect(light_dir, surface_normal);
  float specular_factor = dot(reflected_light_v, v_cameratofrag);
  specular_factor = max(specular_factor, 0.0);
  float damped_factor = pow(specular_factor, material.shininess);
  vec3 specular = damped_factor * reflectivity * dir_light.specular;
  return DirlightColors(diffuse, specular);
}

void main()
{
  clip_check(v_clipdistance);

  vec3 ambient = u_ambient.color * u_material.ambient;

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  DirlightColors dirlight_colors = calc_dirlight_special(u_directional_light, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, v_surfacenormal);
  vec3 dirlight = dirlight_colors.diffuse + dirlight_colors.specular;

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

  vec2 distortion = texture(u_dudv_sampler, vec2(v_dudv.x + u_dudv_offset, v_dudv.y)).rg * 0.1;
  distortion = v_dudv + vec2(distortion.x, distortion.y + u_dudv_offset);
  distortion = (texture(u_dudv_sampler, distortion).rg * 2.0 - 1.0) * u_wavestrength;

  reflect_uv += distortion;
  refract_uv += distortion;

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
  vec4 texture_color = texture(u_texture_sampler, vec2(v_diffuseuv.x + u_diffuse_offset, v_diffuseuv.y + u_diffuse_offset)) * weight_texture;
  light_color        = light_color * weight_light;

  // nc === normal color
  vec4 nc = texture(u_normal_sampler, v_diffuseuv);
  vec3 normal = vec3(nc.r * 2.0 - 1.0, nc.b, nc.g * 2.0 - 1.0);

  vec3 reflected_light = reflect(normalize(u_directional_light.direction), normal);
  float specular = max(dot(reflected_light, view_vector), 0.0);
  specular = pow(specular, u_material.shininess);
  vec3 specular_highlights = dirlight_colors.diffuse * dirlight_colors.specular * u_reflectivity;

  fragment_color = texture_color + effect_color + texture_color + light_color;

  const vec4 BLUE_MIX = vec4(0.0, 0.3, 0.8, 1.0);
  fragment_color = mix(fragment_color, BLUE_MIX, 0.6) + vec4(specular_highlights, 0.0);
}
