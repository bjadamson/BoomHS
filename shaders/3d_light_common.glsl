#define MAX_NUM_POINTLIGHTS 4

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct GlobalLight {
  vec3 ambient;
};

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

float
calculate_attenuation(PointLight light, vec3 frag_world_position)
{
  float distance = length(light.position - frag_world_position);

  float constant = light.attenuation.constant;
  float linear = light.attenuation.linear * distance;
  float quadratic = light.attenuation.quadratic * (distance * distance);

  float denominator = constant + linear + quadratic;
  float attenuation = 1.0 / denominator;
  return attenuation;
}

vec3
calc_pointlight(PointLight light, vec3 v_lighttofrag, vec3 frag_world_position,
    mat4 invviewmatrix, Material material, float reflectivity,
    vec3 surface_normal)
{
  float dotp = dot(surface_normal, v_lighttofrag);
  float brightness = max(dotp, 0.0);
  vec3 diffuse = brightness * light.diffuse * material.diffuse;

  vec3 light_direction = -v_lighttofrag;
  vec3 reflected_light_v = reflect(light_direction, surface_normal);

  vec3 camera_position = (invviewmatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  vec3 v_cameratofrag = normalize(camera_position - frag_world_position);

  float specular_factor = dot(reflected_light_v, v_cameratofrag);
  specular_factor = max(specular_factor, 0.0);
  float damped_factor = pow(specular_factor, material.shininess);
  vec3 specular = damped_factor * reflectivity * light.specular;

  float attenuation = calculate_attenuation(light, frag_world_position);
  diffuse *= attenuation;
  specular *= attenuation;

  return diffuse + specular;
}

vec3
calc_pointlights(PointLight pointlights[MAX_NUM_POINTLIGHTS], mat4 modelmatrix, vec4 position,
    mat4 invviewmatrix, Material material, float reflectivity,
    vec3 surface_normal)
{
  vec3 frag_world_position = (modelmatrix * position).xyz;
  vec3 color = vec3(0.0);
  for(int i = 0; i < MAX_NUM_POINTLIGHTS; i++) {
    vec3 light_to_frag = normalize(pointlights[i].position - frag_world_position);
    color += calc_pointlight(pointlights[i], light_to_frag, frag_world_position,
      invviewmatrix, material, reflectivity, surface_normal);
  }
  return color;
}

