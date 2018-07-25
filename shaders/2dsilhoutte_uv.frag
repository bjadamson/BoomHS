in vec2 v_uv;

uniform sampler2D u_sampler;
uniform vec3 u_color;

out vec4 fragment_color;

void main()
{
  vec4 sample_value = texture(u_sampler, vec2(v_uv.x, v_uv.y));

  fragment_color = vec4(u_color, sample_value.a) * sample_value;
}
