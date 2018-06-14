in vec4 v_position;
in vec3 v_surfacenormal;
in vec2 v_uv;
in float v_visibility;
in float clip_distance;

uniform sampler2D u_bgsampler;
uniform sampler2D u_rsampler;
uniform sampler2D u_gsampler;
uniform sampler2D u_bsampler;
uniform sampler2D u_blendsampler;

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

uniform float u_uvmodifier;

out vec4 fragment_color;

void main()
{
  clip_check(clip_distance);

  vec4 blendmap_color = texture(u_blendsampler, v_uv);
  float back_texture_amount = 1.0 - (blendmap_color.r + blendmap_color.g + blendmap_color.b);
  vec2 tiled_coords = v_uv * u_uvmodifier;
  vec4 background_texcolor = texture(u_bgsampler, tiled_coords) * back_texture_amount;
  vec4 r_texcolor    = texture(u_rsampler, tiled_coords) * blendmap_color.r;
  vec4 g_texcolor    = texture(u_gsampler, tiled_coords) * blendmap_color.g;
  vec4 b_texcolor    = texture(u_bsampler, tiled_coords) * blendmap_color.b;
  vec4 texture_color = background_texcolor + r_texcolor + g_texcolor + b_texcolor;

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
  }
  else if (u_ignore_dirlight == 1) {
    fragment_color = vec4(ambient_pointlight, 1.0) * texture_color;
  }
  else {
    fragment_color = vec4(light, 1.0) * texture_color;
  }
  fragment_color = mix(u_fog.color, fragment_color, v_visibility);
}
