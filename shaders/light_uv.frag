precision mediump float;
in vec4 v_color;
in vec2 v_uv;

uniform sampler2D u_sampler;

out vec4 fragment_color;

void main()
{
  vec4 texture_color = texture(u_sampler, v_uv);
  fragment_color = v_color * texture_color;
}
