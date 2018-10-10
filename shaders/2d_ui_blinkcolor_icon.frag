in vec2 v_uv;

uniform sampler2D u_sampler;
uniform vec4 u_blinkcolor;

out vec4 fragment_color;

void main()
{
  fragment_color = texture(u_sampler, vec2(v_uv.x, v_uv.y));
  fragment_color = mix(fragment_color, u_blinkcolor, 1.0 - fragment_color.a);
}
