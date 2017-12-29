// attributes input to the vertex shader
in vec4 a_position; // position value

// output of the vertex shader - input to fragment
// shader
out vec3 v_uv;

uniform mat4 u_mvpmatrix;

void main()
{
  gl_Position = u_mvpmatrix * a_position;
  v_uv = vec3(a_position.x, a_position.y, a_position.z);
}
