in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec3 v_worldposition;
out vec3 v_surfacenormal;
out vec4 v_color;

#define MAX_NUM_POINTLIGHTS 1
out vec3 v_tolights[MAX_NUM_POINTLIGHTS];
out vec3 v_tocamera;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat4 u_normalmatrix;
uniform mat4 u_viewmatrix;

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

void main()
{
  gl_Position = u_mvpmatrix * a_position;

  vec3 v_normal = (u_normalmatrix * vec4(a_normal, 0.0)).xyz;
  v_surfacenormal = normalize((u_modelmatrix * vec4(v_normal, 0.0)).xyz);

  v_worldposition = (u_modelmatrix * a_position).xyz;
  vec3 camera_position = (inverse(u_viewmatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  v_tocamera = normalize(camera_position - v_worldposition);
  v_color = a_color;

  for(int i = 0; i < MAX_NUM_POINTLIGHTS; i++) {
    v_tolights[i] = normalize(u_pointlights[i].position - v_worldposition);
  }
}
