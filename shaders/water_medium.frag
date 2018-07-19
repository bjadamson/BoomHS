in vec4 v_position;
in vec2 v_textureuv;
in float v_visibility;
in float v_clipdistance;
in vec4 v_clipspace;

uniform sampler2D u_diffuse_sampler;
uniform sampler2D u_normal_sampler;

uniform Material         u_material;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform AmbientLight     u_ambient;
uniform DirectionalLight u_directional_light;

uniform int   u_drawnormals;
uniform int   u_ignore_dirlight;
uniform float u_reflectivity;

uniform Fog u_fog;
uniform Water u_water;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;
uniform float u_time_offset;

out vec4 fragment_color;

void main()
{
  clip_check(v_clipdistance);

  // nc === normal color
  vec4 nc = texture(u_normal_sampler, v_textureuv + u_time_offset);
  vec3 surface_normal = vec3(nc.r, -(nc.g / 2.0), nc.b);
  surface_normal = normalize(surface_normal);

  vec3 ambient = u_ambient.color * u_material.ambient;

  vec3 frag_world_pos = (u_modelmatrix * v_position).xyz;
  vec3 dirlight = calc_dirlight(u_directional_light, u_material, u_reflectivity, frag_world_pos,
      u_invviewmatrix, surface_normal);

  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, surface_normal);

  vec3 ambient_pointlight = ambient + pointlights;
  vec3 light = ambient_pointlight + dirlight;
  vec4 light_color = vec4(light, 1.0);

  if (u_drawnormals == 1) {
    light_color = vec4(surface_normal, 1.0);
  }
  else if (u_ignore_dirlight == 1) {
    light_color = vec4(ambient_pointlight, 1.0) * light_color;
  }
  else {
    light_color = vec4(light, 1.0) * light_color;

    const float weight_light   = 1.0;
    const float weight_texture = 1.0;

    vec4 texture_color = weighted_texture_sample(u_diffuse_sampler, v_textureuv, u_time_offset, weight_texture);
    light_color        = light_color * weight_light;

    fragment_color = texture_color + light_color;

    // add additional color to the water texture
    fragment_color = mix_in_water_and_fog(fragment_color, u_water, u_fog, v_visibility);
  }
}
