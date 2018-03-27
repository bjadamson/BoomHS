// attributes input to the vertex shader
in vec4 a_position; // position value
in vec3 a_normal;
in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec4 v_position;
out vec3 v_surfacenormal;
out vec2 v_uv;

uniform mat4 u_mvpmatrix;
uniform mat3 u_normalmatrix;

void main()
{
  v_position = a_position;
  gl_Position = u_mvpmatrix * v_position;

  v_surfacenormal = normalize(u_normalmatrix * a_normal);
  v_uv = a_uv;
}
