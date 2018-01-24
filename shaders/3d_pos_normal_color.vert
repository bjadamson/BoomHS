in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat4 u_normalmatrix;

void main()
{
  v_position = a_position;
  gl_Position = u_mvpmatrix * v_position;

  mat3 normal_matrix = mat3(transpose(inverse(u_modelmatrix)));
  v_surfacenormal = normalize((normal_matrix * a_normal).xyz);

  ////////vec3 v_normal = (u_normalmatrix * vec4(a_normal, 0.0)).xyz;
  ////////v_surfacenormal = normalize((u_modelmatrix * vec4(v_normal, 0.0)).xyz);

  v_color = a_color;
}
