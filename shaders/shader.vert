// attributes input to the vertex shader
in vec4 a_position; // position value
in vec4 a_color;
in vec2 a_uv;

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;
out vec2 v_uv;

uniform mat4 view;
uniform mat4 projection;

void main()
{
  gl_Position = projection * view * a_position;
  v_color = a_color;
  v_uv = a_uv;
}
