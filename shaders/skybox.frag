in vec3 v_uv;

out vec4 fragment_color;

uniform samplerCube u_sampler;
uniform Fog u_fog;

const float limit_lower = 0.0;
const float limit_upper = 30.0;

void main()
{
  vec4 texture_color = texture(u_sampler, v_uv);

  float factor = (v_uv.y - limit_lower) / (limit_upper - limit_lower);
  factor = clamp(factor, 0.0, 1.0);

  fragment_color = mix(texture_color, u_fog.color, factor);
}
