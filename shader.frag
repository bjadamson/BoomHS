#version 300 es
precision mediump float;

in vec4 v_color;
in vec2 uv;

out vec4 f_color;  // output fragment color

uniform sampler2D sampler;

void main()
{
  f_color = vec4(texture(sampler, uv).rgb, 1.0f);
}
