// attributes input to the vertex shader
layout (location = VERTEX_ATTRIBUTE_INDEX_OF_POSITION) in vec4 a_position; // position value
layout (location = VERTEX_ATTRIBUTE_INDEX_OF_COLOR) in vec4 a_color;
layout (location = VERTEX_ATTRIBUTE_INDEX_OF_UV) in vec2 a_uv;

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
