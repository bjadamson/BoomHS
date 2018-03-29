in vec2 v_uv;

uniform sampler2D u_sampler;

out vec4 fragment_color;

void main()
{
  vec4 texture_color = texture(u_sampler, v_uv);
  fragment_color = texture_color;
}
