precision mediump float;

uniform vec4 u_color;

out vec4 fragment_color;  // output fragment color

void main()
{
  fragment_color = vec4(1.0, 1.0, u_color.z, 1.0);
}
