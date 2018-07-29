in vec2 v_uv;

uniform sampler2D u_sampler;
uniform vec4 u_blendcolor;
out vec4 fragment_color;

void main()
{
  vec2 pos = vec2(v_uv.x, 1.0 - v_uv.y);
  fragment_color = texture(u_sampler, pos);

  const float offset = 1.0;
  fragment_color = mix(fragment_color, u_blendcolor, fragment_color.a);
}
