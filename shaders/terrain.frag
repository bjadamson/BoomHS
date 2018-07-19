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

  vec4 light_color = calculate_ambient_dirlight_pointlight_color(u_ambient, u_directional_light,
     u_pointlights, u_material, u_modelmatrix, u_invviewmatrix, v_position, u_reflectivity,
     v_surfacenormal);
  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  }
  else {
    fragment_color = light_color * texture_color;
  }
  fragment_color = mix(u_fog.color, fragment_color, v_visibility);
}
