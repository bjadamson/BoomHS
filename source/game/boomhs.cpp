#include <game/boomhs.hpp>

// TODO: remove this, THIS is the game loop code and this is sdl specific.
#include <engine/window/sdl_window.hpp>
#include <engine/gfx/shader.hpp>
#include <stlw/type_ctors.hpp>

namespace game
{

stlw::result<stlw::empty_type, std::string>
boomhs::game_loop(boomhs::window_type &&window)
{
  bool quit = false;
  SDL_Event sdlEvent;

  // Create and compile our GLSL program from the shaders
  // TODO: decide if it's the game loop's responsibility to load the shaders (I'm guessing not)
  auto const expected_program = engine::gfx::load_shaders("shader.vert", "shader.frag");
  if (! expected_program) {
    std::cerr << "expected_program is error '" << expected_program.error() << "'\n";
    quit = true;
  }
  GLuint const programID = *expected_program;
  auto log_error = [](auto const line)
  {
    GLenum err = GL_NO_ERROR;
    while((err = glGetError()) != GL_NO_ERROR) {
      std::cerr << "GL error! '" << std::hex << err << "' line #" << std::to_string(line) << std::endl;
      return;
    }
    std::string error = SDL_GetError();
    if (error != "") {
      std::cout << "SLD Error : " << error << std::endl;
      SDL_ClearError();
    }
  };

  // Set up vertex data (and buffer(s)) and attribute pointers
  constexpr GLfloat W = 1.0f;
  constexpr GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f, W,
     0.5f, -0.5f, 0.0f, W,
     0.0f,  0.5f, 0.0f, W
  };
  GLuint VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

  glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs)

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  uint32_t ticks = SDL_GetTicks(), lastticks = 0;
  while (!quit) {
    while(SDL_PollEvent(&sdlEvent) != 0) {
    log_error(__LINE__);
      // Esc button is pressed
      if(sdlEvent.type == SDL_QUIT) {
        quit = true;
      }
    }
    // Render
    log_error(__LINE__);
    glClear(GL_COLOR_BUFFER_BIT);
    log_error(__LINE__);
    ticks = SDL_GetTicks();
    if ( ((ticks*10-lastticks*10)) < 167 ) {
      SDL_Delay( (167-((ticks*10-lastticks*10)))/10 );
      log_error(__LINE__);
    }
    lastticks = SDL_GetTicks();
    log_error(__LINE__);

    // Draw our first triangle
    glUseProgram(programID);
    log_error(__LINE__);

    char buffer[2096];
    int actual_length = 0;
    glGetProgramInfoLog(programID, 2096, &actual_length, buffer);
    if (0 < actual_length) {
      std::cerr << "log: '" << std::to_string(buffer[0]) << "'\n";
    }

    log_error(__LINE__);
    glBindVertexArray(VAO);
    log_error(__LINE__);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    log_error(__LINE__);
    glBindVertexArray(0);
    log_error(__LINE__);

// ********************************** END
    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());
    log_error(__LINE__);
    SDL_Delay(20);
    log_error(__LINE__);
  }

  // Properly de-allocate all resources once they've outlived their purpose
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(programID);

  return stlw::make_empty();
}

} // ns game
