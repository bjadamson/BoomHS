in vec2 v_uv;

uniform sampler2D u_sampler;

// The center (in screen coordinates) of the light source
uniform DirectionalLight u_dirlight;

// The width of the blur (the smaller it is the further each pixel is going to sample)
const float blurWidth = -0.85;

#define NUM_SAMPLES 50

out vec4 fragment_color;

void main()
{
  // compute ray from pixel to light center
  vec2 sun_pos = u_dirlight.screenspace_pos;

  vec2 ray = v_uv - sun_pos;

  // output color
  vec3 color = vec3(0.0);

  // sample the texture NUM_SAMPLES times
  for(int i = 0; i < NUM_SAMPLES; i++) {
    // sample the texture on the pixel-to-center ray getting closer to the center every iteration
    float scale = 1.0 + blurWidth * (float(i) / float(NUM_SAMPLES - 1));

    // summing all the samples togheter
    color += (texture2D(u_sampler, (ray * scale) + sun_pos).xyz) / float(NUM_SAMPLES);
  }
  fragment_color = vec4(color, 1.0);
}
