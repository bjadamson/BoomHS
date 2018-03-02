precision mediump float;
in vec2 v_uv;

uniform sampler2D u_sampler;
uniform vec3 u_lightcolor;
uniform float u_glow;

out vec4 fragment_color;

void main()
{
  float intensity = u_glow;
  vec3 rgb = intensity * u_lightcolor;

  fragment_color = vec4(rgb, 1.0) * texture(u_sampler, v_uv);
}
