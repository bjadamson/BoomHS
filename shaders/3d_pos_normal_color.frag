precision mediump float;

in vec3 v_normal;
in vec4 v_color;
in vec3 v_fragpos_worldspace;

uniform vec3 u_viewpos;

struct Player {
  vec3 position;
  vec3 direction;
  float cutoff;
};

uniform Player u_player;

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};
uniform Material u_material;

struct Light {
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};
uniform Light u_light;

out vec4 fragment_color;

void main()
{
  vec3 light_dir = normalize(u_light.position - v_fragpos_worldspace);
  float theta = dot(light_dir, -u_player.direction);

  // ambient
    vec3 ambient = u_light.ambient * u_material.ambient;

  if(theta > u_player.cutoff) {
    // diffuse
    vec3 norm = normalize(v_normal);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * (u_light.diffuse * u_material.diffuse);

    // specular
    vec3 view_dir = normalize(u_viewpos - v_fragpos_worldspace);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_material.shininess);
    vec3 specular = u_light.specular * spec * u_material.specular;

    float distance    = length(u_player.position - v_fragpos_worldspace);
    float attenuation = 1.0 / (u_light.constant + u_light.linear * distance + u_light.quadratic * (distance * distance));

    diffuse  *= attenuation;
    specular *= attenuation;
    vec4 result = vec4(ambient + diffuse + specular, 1.0) * v_color;
    fragment_color = result;
  } else {
    // else, use ambient light so scene isn't completely dark outside the spotlight.
    //fragment_color = vec4(ambient, 1.0);
    fragment_color = vec4(0.0);//u_light.ambient * vec3(texture(material.diffuse, TexCoords)), 1.0);
  }
}
