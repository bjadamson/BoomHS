precision mediump float;

in vec3 v_normal;
in vec4 v_color;
in vec3 v_fragpos_worldspace;

uniform vec3 u_lightcolor;
uniform vec3 u_lightpos;
uniform vec3 u_viewpos;

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

uniform Material u_material;

out vec4 fragment_color;

void main()
{
  // ambient
  vec3 ambient = u_lightcolor * u_material.ambient;

  // diffuse
  vec3 norm = normalize(v_normal);
  vec3 light_dir = normalize(u_lightpos - v_fragpos_worldspace);
  float diff = max(dot(norm, light_dir), 0.0);
  vec3 diffuse = diff * (u_lightcolor * u_material.diffuse);

  // specular
  vec3 view_dir = normalize(u_viewpos - v_fragpos_worldspace);
  vec3 reflect_dir = reflect(-light_dir, norm);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_material.shininess);
  vec3 specular = u_lightcolor * spec * u_material.specular;

  // result
  vec4 result = vec4(u_material.ambient + diffuse + specular, 1.0) * v_color;
  fragment_color = result;
}
