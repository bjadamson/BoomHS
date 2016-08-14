#version 300 es
precision mediump float;

in vec4 vertex_color;
out vec4 fragment_color;  // output fragment color

void main()
{
  // Output color = red
  fragment_color = vertex_color;
}
