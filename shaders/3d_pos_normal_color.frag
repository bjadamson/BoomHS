in vec4 v_position;
in vec3 v_surfacenormal;
in vec4 v_color;
in float v_visibility;

uniform Material         u_material;
uniform AmbientLight     u_ambient;
uniform PointLight       u_pointlights[MAX_NUM_POINTLIGHTS];
uniform DirectionalLight u_directional_light;

uniform int   u_drawnormals;
uniform float u_reflectivity;

uniform Fog u_fog;
uniform mat4 u_modelmatrix;
uniform mat4 u_invviewmatrix;

out vec4 fragment_color;

void main()
{
  vec4 light_color = calculate_ambient_dirlight_pointlight_color(u_ambient, u_directional_light,
     u_pointlights, u_material, u_modelmatrix, u_invviewmatrix, v_position, u_reflectivity,
     v_surfacenormal);

  if (u_drawnormals == 1) {
    fragment_color = vec4(v_surfacenormal, 1.0);
  }
  else {
    fragment_color = light_color * v_color;
  }
  fragment_color = mix(u_fog.color, fragment_color, v_visibility);
}
