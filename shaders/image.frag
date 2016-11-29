precision mediump float;

in vec2 v_uv;

uniform sampler2D sampler;
out vec4 fragment_color;  // output fragment color

void main()
{
  fragment_color = texture(sampler, v_uv);
}
