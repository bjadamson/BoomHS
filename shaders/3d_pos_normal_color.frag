precision mediump float;

in vec3 v_normal;
in vec4 v_color;
in vec3 v_fragpos_worldspace;

uniform vec4 u_ambient;
uniform vec4 u_diffuse_color;
uniform vec3 u_diffuse_pos;

out vec4 fragment_color;

void main()
{
  vec3 norm = normalize(v_normal);
  vec3 light_dir = normalize(u_diffuse_pos - v_fragpos_worldspace);

  float diff = max(dot(norm, light_dir), 0.0);
  vec4 diffuse = diff * u_diffuse_color;
  vec4 result = (u_ambient + diffuse) * v_color;

  fragment_color = result;
}
