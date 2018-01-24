precision mediump float;

in vec4 v_position;
in vec3 v_surfacenormal;
in vec4 v_color;

#define MAX_NUM_POINTLIGHTS 4
in vec3 v_lightstofrag[MAX_NUM_POINTLIGHTS];

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

uniform PointLight u_pointlights[MAX_NUM_POINTLIGHTS];

uniform Material u_material;
uniform GlobalLight u_globallight;
uniform float u_reflectivity;
uniform mat4 u_modelmatrix;
uniform mat4 u_viewmatrix;
uniform int u_drawnormals;

out vec4 fragment_color;

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
calc_pointlight(PointLight light, vec3 v_lighttofrag, vec3 frag_world_position)
{
  float dotp = dot(v_surfacenormal, v_lighttofrag);
  float brightness = max(dotp, 0.0);
  vec3 diffuse = brightness * light.diffuse * u_material.diffuse;

  vec3 light_direction = -v_lighttofrag;
  vec3 reflected_light_v = reflect(light_direction, v_surfacenormal);

  vec3 camera_position = (inverse(u_viewmatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  vec3 v_cameratofrag = normalize(camera_position - frag_world_position);

  float specular_factor = dot(reflected_light_v, v_cameratofrag);
  specular_factor = max(specular_factor, 0.0);
  float damped_factor = pow(specular_factor, u_material.shininess);
  vec3 specular = damped_factor * u_reflectivity * light.specular;

  float attenuation = calculate_attenuation(light, frag_world_position);
  diffuse *= attenuation;
  specular *= attenuation;

  return diffuse + specular;
}

void main()
{
  vec3 ambient = u_globallight.ambient * u_material.ambient;
  vec3 frag_world_position = (u_modelmatrix * v_position).xyz;

  vec3 pointlights = vec3(0.0);
  for(int i = 0; i < MAX_NUM_POINTLIGHTS; i++) {
    vec3 light_to_frag = normalize(u_pointlights[i].position - frag_world_position);
    pointlights += calc_pointlight(u_pointlights[i], light_to_frag, frag_world_position);
  }

  vec3 light = ambient + pointlights;
  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  } else {
    fragment_color = vec4(light, 1.0) * v_color;
  }
}
