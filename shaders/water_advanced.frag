in vec4 v_position;
in vec2 v_textureuv;
in float v_visibility;
in float v_clipdistance;
in vec4 v_clipspace;
in vec2 v_fbouv;
in vec3 v_tocamera;

uniform sampler2D u_diffuse_sampler;
uniform sampler2D u_reflect_sampler;
uniform sampler2D u_refract_sampler;
uniform sampler2D u_dudv_sampler;
uniform sampler2D u_normal_sampler;
uniform sampler2D u_depth_sampler;

uniform Material         u_material;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform AmbientLight     u_ambient;
uniform DirectionalLight u_dirlight;

uniform int   u_drawnormals;
uniform float u_reflectivity;

uniform float u_far;
uniform float u_near;
uniform float u_fresnel_reflect_power;
uniform float u_depth_divider;

uniform Fog u_fog;
uniform Water u_water;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

// water related
uniform float u_wave_offset;
uniform float u_wavestrength;
uniform float u_time_offset;
uniform vec2  u_flowdir;

out vec4 fragment_color;

void main()
{
  clip_check(v_clipdistance);

  // nc === normal color
  vec4 nc = texture(u_normal_sampler, v_textureuv + u_time_offset);
  vec3 surface_normal = vec3(nc.r, -(nc.g / 2.0), nc.b);
  surface_normal = normalize(surface_normal);

  vec4 light_color = calculate_ambient_dirlight_pointlight_color(u_ambient, u_dirlight,
     u_pointlights, u_material, u_modelmatrix, u_invviewmatrix, v_position, u_reflectivity,
     surface_normal);

  if (u_drawnormals == 1) {
    light_color = vec4(surface_normal, 1.0);
  }
  else {
    vec2 ndc         = ((v_clipspace.xy/v_clipspace.w)/2.0) + 0.5;
    ndc.y = 1.0 - ndc.y;
    vec2 reflect_uv  = vec2(ndc.x, -1.0 +ndc.y);
    vec2 refract_uv  = vec2(ndc.x, 1.0 - ndc.y);

    // depth info is stored in r component
    float depth = texture(u_depth_sampler, refract_uv).r;
    float ground_distance = 2.0 * u_near * u_far / (u_far + u_near - (2.0 * depth - 1.0) * (u_far - u_near));
    depth = gl_FragCoord.z;
    float water_distance = 2.0 * u_near * u_far / (u_far + u_near - (2.0 * depth - 1.0) * (u_far - u_near));
    float water_depth = ground_distance - water_distance;

    vec2 distortion = texture(u_dudv_sampler, vec2(v_fbouv.x + u_wave_offset, v_fbouv.y)).rg * 0.1;
    distortion = v_fbouv + vec2(distortion.x, distortion.y + u_wave_offset);
    distortion = (texture(u_dudv_sampler, distortion).rg * 2.0 - 1.0);
    distortion *= u_wavestrength;
    distortion *= clamp(water_depth / u_depth_divider, 0.0, 1.0);

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
    float refractive_factor = dot(view_vector, surface_normal);

    refractive_factor = pow(refractive_factor, u_fresnel_reflect_power);

    vec4 effect_color  = mix(reflect_color, refract_color, refractive_factor) * u_water.weight_mix_effect;
    vec4 texture_color = weighted_texture_sample(u_diffuse_sampler, v_textureuv, u_flowdir,
                                                 u_time_offset, u_water.weight_texture);
    light_color        = light_color * u_water.weight_light;

    fragment_color = effect_color + texture_color + light_color;

    // add additional color to the water texture
    fragment_color = mix_in_water_and_fog(fragment_color, u_water, u_fog, v_visibility);
  }
}
