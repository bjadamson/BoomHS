precision mediump float;

in vec3 v_normal;
in vec2 v_uv;

uniform sampler2D u_sampler;
out vec4 fragment_color;  // output fragment color

void main()
{
  fragment_color = texture(u_sampler, v_uv);
}
