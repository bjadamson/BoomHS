precision mediump float;

in vec3 v_normal;
in vec4 v_color;
in vec3 v_fragpos_worldspace;

uniform vec4 u_ambient;
uniform vec4 u_diffuse_color;
uniform vec3 u_diffuse_pos;
uniform vec3 u_viewpos;
uniform float u_specularstrength;

out vec4 fragment_color;

void main()
{
  vec3 norm = normalize(v_normal);
  vec3 light_dir = normalize(u_diffuse_pos - v_fragpos_worldspace);

  float diff = max(dot(norm, light_dir), 0.0);
  vec4 diffuse = diff * u_diffuse_color;

  vec3 view_dir = normalize(u_viewpos - v_fragpos_worldspace);
  vec3 reflect_dir = reflect(-light_dir, norm);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
  vec4 specular = u_specularstrength * spec * u_diffuse_color;
  vec4 result = (u_ambient + diffuse + specular) * v_color;

  fragment_color = result;
}
