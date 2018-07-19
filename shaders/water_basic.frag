in vec4 v_position;
in vec2 v_textureuv;
in float v_visibility;
in float v_clipdistance;
in vec4 v_clipspace;

uniform sampler2D u_diffuse_sampler;
uniform sampler2D u_normal_sampler;

uniform int   u_drawnormals;

uniform Fog u_fog;
uniform mat4 u_modelmatrix;
uniform float u_time_offset;

out vec4 fragment_color;

void main()
{
  clip_check(v_clipdistance);

  // nc === normal color
  vec4 nc = texture(u_normal_sampler, v_textureuv + u_time_offset);
  vec3 surface_normal = vec3(nc.r, -(nc.g / 2.0), nc.b);
  surface_normal = normalize(surface_normal);

  if (u_drawnormals == 1) {
    fragment_color = vec4(surface_normal, 1.0);
  }
  else {
    const float weight_texture = 1.0;
    fragment_color = texture(u_diffuse_sampler, v_textureuv + u_time_offset) * weight_texture;

    const vec4 BLUE_MIX = vec4(0.0, 0.3, 0.8, 1.0);
    fragment_color = mix(fragment_color, BLUE_MIX, 0.6);

    // mix the fog into the fragment color
    fragment_color = mix(u_fog.color, fragment_color, v_visibility);
  }
}
