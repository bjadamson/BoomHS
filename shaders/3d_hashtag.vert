in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;

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

void main()
{
  const vec3 vertical_offsets[3] = vec3[3]( vec3(0.0, 0.0, 0.0), vec3(0.0, 0.1, 0.0), vec3(0.0, 0.2, 0.0) );
  const vec3 colors[3] = vec3[3]( vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0) );
  vec3 offset = vertical_offsets[gl_InstanceID];
  vec4 color = vec4(colors[gl_InstanceID], 1.0);

  v_position = a_position;
  gl_Position = u_mvpmatrix * (a_position + vec4(offset, 1.0));

  vec3 v_normal = (u_normalmatrix * vec4(a_normal, 0.0)).xyz;
  v_surfacenormal = normalize((u_modelmatrix * vec4(v_normal, 0.0)).xyz);

  v_color = a_color;
}
