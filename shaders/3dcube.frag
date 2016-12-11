precision mediump float;

in vec3 v_uv;

uniform samplerCube u_sampler;
out vec4 fragment_color;  // output fragment color

void main()
{
  fragment_color = texture(u_sampler, v_uv);
}