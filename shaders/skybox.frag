in vec3 v_uv;

out vec4 fragment_color;

uniform samplerCube u_cube_sampler0;
uniform samplerCube u_cube_sampler1;
uniform Fog u_fog;
uniform float u_blend_factor;

const float limit_lower = 0.0;
const float limit_upper = 0.3;

void main()
{
  vec4 day   = texture(u_cube_sampler0, v_uv);
  vec4 night = texture(u_cube_sampler1, v_uv);
  vec4 texture_color = mix(day, night, u_blend_factor);

  // Calculate a value that will cause the fog to be near 1.0 around the center of the screen
  // (controlled with limit_upper and limit_lower)
  float factor = (v_uv.y - limit_lower) / (limit_upper - limit_lower);
  factor = clamp(factor, 0.0, 1.0);

  fragment_color = mix(u_fog.color, texture_color, factor);
}
