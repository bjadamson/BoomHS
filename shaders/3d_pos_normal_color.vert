in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

out vec4 v_position;
out vec3 v_surfacenormal;
out vec4 v_color;

uniform mat4 u_mvpmatrix;
uniform mat4 u_modelmatrix;
uniform mat3 u_normalmatrix;

void main()
{
  v_position = a_position;
  gl_Position = u_mvpmatrix * v_position;

  mat3 normal_matrix = mat3(transpose(inverse(u_modelmatrix)));
  //v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_surfacenormal = normalize(normal_matrix * a_normal);

  v_color = a_color;
}
