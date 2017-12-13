// attributes input to the vertex shader
in vec4 a_position; // position value
in vec4 a_color;

// output of the vertex shader - input to fragment
// shader
out vec4 v_color;

uniform mat4 u_mvmatrix;
uniform vec3 u_offset;

void main()
{
  const vec3 vertical_offsets[3] = vec3[3]( vec3(0.0, 0.0, 0.0), vec3(0.0, 0.2, 0.0), vec3(0.0, 0.4, 0.0) );
  const vec3 colors[3] = vec3[3]( vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0) );
  vec3 offset = vertical_offsets[gl_InstanceID] + u_offset;
  vec4 color = vec4(colors[gl_InstanceID], 1.0);

  vec4 pos = a_position + vec4(offset, 1.0);
  gl_Position = u_mvmatrix * pos;
  v_color = color;
}
