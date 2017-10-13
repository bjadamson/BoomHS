// attributes input to the vertex shader
in vec4 a_position; // position value
in vec3 a_normal;
in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec3 v_normal;
out vec2 v_uv;

uniform mat4 u_mvmatrix;

void main()
{
  gl_Position = u_mvmatrix * a_position;
  v_normal = a_normal;
  v_uv = a_uv;
}
