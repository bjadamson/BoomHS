precision mediump float;

in vec3 v_normal;
in vec4 v_color;
in vec3 v_fragpos_worldspace;

uniform vec3 u_viewpos;

//struct Player {
  //vec3 position;
  //vec3 direction;
  //float cutoff;
//};

//uniform Player u_player;

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};
uniform Material u_material;

struct GlobalLight {
  vec3 ambient;
};
uniform GlobalLight u_globallight;

struct GlobalDirLight {
  vec3 direction;

  vec3 diffuse;
  vec3 specular;
};
uniform GlobalDirLight u_dirlight;

struct LightAttenuation
{
  float constant;
  float linear;
  float quadratic;
};

struct PointLight {
  vec3 position;

  vec3 diffuse;
  vec3 specular;

  LightAttenuation attenuation;
};

// TODO: somehow generate or read from single source of truth.
#define MAX_NUM_POINTLIGHTS 4
uniform PointLight u_pointlights[MAX_NUM_POINTLIGHTS];

out vec4 fragment_color;

vec3
calc_global_dirlight(GlobalDirLight light, vec3 normal, vec3 view_dir)
{
  vec3 light_dir = normalize(-light.direction);

  // diffuse shading
  float diff = max(dot(normal, light_dir), 0.0);

  // specular shading
  vec3 reflect_dir = reflect(-light_dir, normal);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_material.shininess);

  // combine results
  //vec3 ambient  = light.ambient;//  * vec3(texture(u_material.diffuse, TexCoords));
  vec3 diffuse  = light.diffuse  * diff;// * vec3(texture(u_material.diffuse, TexCoords));
  vec3 specular = light.specular * spec;// * vec3(texture(u_material.specular, TexCoords));
  return (diffuse + specular);
}

float
calculate_attenuation(PointLight light)
{
  float distance = length(light.position - v_fragpos_worldspace);

  float constant = light.attenuation.constant;
  float linear = light.attenuation.linear * distance;
  float quadratic = light.attenuation.quadratic * (distance * distance);

  float denominator = constant + linear + quadratic;
  float attenuation = 1.0 / denominator;
  return attenuation;
}

vec3
calc_pointlight(PointLight light, vec3 normal, vec3 view_dir)
{
  //vec3 light_dir = normalize(light.position - v_fragpos_worldspace);
  //float theta = dot(-light_dir, u_player.direction);

  //float diff_intensity = 5.0f;
  //float diff = max(dot(normal, light_dir), 0.0);

  // TODO: turn into adjustable knob
  //vec3 diffuse = diff_intensity * diff * (light.diffuse * u_material.diffuse);

  // specular
  //vec3 reflect_dir = reflect(-light_dir, normal);
  //float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_material.shininess);
  //vec3 specular = light.specular * spec * u_material.specular;

  //float attenuation = calculate_attenuation(light);

  //diffuse *= attenuation;
  //specular *= attenuation;
  //return diffuse + specular;

    vec3 lightDir = normalize(light.position - v_fragpos_worldspace);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), u_material.shininess);
    // attenuation
    float distance    = length(light.position - v_fragpos_worldspace);
    float attenuation = 1.0 / (light.attenuation.constant + light.attenuation.linear * distance + light.attenuation.quadratic * (distance * distance));

    // combine results
    vec3 diffuse  = light.diffuse  * diff;// * vec3(texture(u_material.diffuse, TexCoords));
    vec3 specular = light.specular * spec;// * vec3(texture(u_material.specular, TexCoords));
    diffuse  *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
}

void main()
{
  vec3 normal = normalize(v_normal);
  vec3 view_dir = normalize(u_viewpos - v_fragpos_worldspace);

  // ambient
  vec3 ambient = u_globallight.ambient * u_material.ambient;

  vec3 pointlight_contribution = calc_global_dirlight(u_dirlight, normal, view_dir);
  for(int i = 0; i < MAX_NUM_POINTLIGHTS; i++) {
    pointlight_contribution += calc_pointlight(u_pointlights[i], normal, view_dir);
  }
  vec4 result = vec4(ambient + pointlight_contribution, 1.0) * v_color;
  fragment_color = result;

  //if(theta > u_player.cutoff) {
  //} else {
    // else, use ambient light so scene isn't completely dark outside the spotlight.
    //fragment_color = vec4(ambient, 1.0);
    //fragment_color = vec4(0.0);//u_dirlight.ambient * vec3(texture(material.diffuse, TexCoords)), 1.0);
  //}
}
