precision mediump float;

in vec4 v_position;
in vec3 v_surfacenormal;

uniform Material    u_material;
uniform PointLight  u_pointlights[MAX_NUM_POINTLIGHTS];
uniform GlobalLight u_globallight;

uniform int   u_drawnormals;
uniform float u_reflectivity;

uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;
uniform vec4 u_color;

out vec4 fragment_color;

void main()
{
  vec3 pointlights = calc_pointlights(u_pointlights, u_modelmatrix, v_position,
      u_invviewmatrix, u_material, u_reflectivity, v_surfacenormal);

  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  } else {
    fragment_color = vec4(pointlights, 1.0) * u_color;
  }
}
