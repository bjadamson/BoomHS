in vec2 v_uv;

uniform sampler2D u_sampler;
out vec4 fragment_color;

void main()
{
  fragment_color = texture(u_sampler, vec2(v_uv.x, 1.0 - v_uv.y));
}
