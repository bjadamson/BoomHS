in vec2 v_uv;

uniform sampler2D u_sampler;
out vec4 fragment_color;

void main()
{
  vec4 sample_value = texture(u_sampler, vec2(v_uv.x, 1.0 - v_uv.y));

  const vec3 BLACK = vec3(0.0, 0.0, 0.0);
  fragment_color = vec4(BLACK, sample_value.a) * sample_value;
}
