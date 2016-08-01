#pragma once
#include <string>

#include <stlw/result.hpp>
#include <game/game.hpp>

// TODO: remove this, THIS is the game loop code and this is sdl specific.
// TODO: from cpp
#include <engine/window/sdl_window.hpp>
#include <engine/gfx/shader.hpp>
#include <stlw/type_ctors.hpp>

namespace game
{
namespace boomhs
{

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

struct boomhs_game
{
  using window_type = ::engine::window::window;

  boomhs_game() = delete;

  static bool
  process_event(SDL_Event &event)
  {
    log_error(__LINE__);
    return event.type == SDL_QUIT;
  }

  static decltype(auto)
  loop_once(window_type &window)
  {
    SDL_Event event;
    bool quit = false;
    while (0 != SDL_PollEvent(&event)) {
      quit = process_event(event);
    }

    // don't quit
    return quit;
  }

  static void
  render(window_type &window, GLuint const program_id, GLuint const VAO)
  {
    // Render
    log_error(__LINE__);
    glClear(GL_COLOR_BUFFER_BIT);
    log_error(__LINE__);

    uint32_t ticks = SDL_GetTicks(), lastticks = 0;
    ticks = SDL_GetTicks();
    if ( ((ticks*10-lastticks*10)) < 167 ) {
      SDL_Delay( (167-((ticks*10-lastticks*10)))/10 );
      log_error(__LINE__);
    }
    lastticks = SDL_GetTicks();
    log_error(__LINE__);

    /////////////////////////////////////////////////////////////////////////////////

    // Draw our first triangle
    glUseProgram(program_id);
    log_error(__LINE__);

    char buffer[2096];
    int actual_length = 0;
    glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
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

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());
    log_error(__LINE__);
    SDL_Delay(20);
    log_error(__LINE__);
  }

  static stlw::result<stlw::empty_type, std::string>
  game_loop(window_type &&window)
  {
    // Set up vertex data (and buffer(s)) and attribute pointers
    constexpr GLfloat W = 1.0f;
    constexpr GLfloat vertices[] = {
      -0.5f, -0.5f, 0.0f, W,
      0.5f, -0.5f, 0.0f, W,
      0.0f,  0.5f, 0.0f, W
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    ON_SCOPE_EXIT([&]() { glDeleteVertexArrays(1, &VAO); });

    glGenBuffers(1, &VBO);
    ON_SCOPE_EXIT([&]() { glDeleteBuffers(1, &VBO); });

    auto const send_vertices_gpu = [](auto const& vbo, auto const& vertices)
    {
      // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), static_cast<GLvoid*>(nullptr));

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0); // Unbind when we are done with this scope automatically.
    };
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);

    send_vertices_gpu(VBO, vertices);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: decide if it's the game loop's responsibility to load the shaders (I'm guessing not)
    DO_MONAD(GLuint const program_id, engine::gfx::load_shaders("shader.vert", "shader.frag"));
    ON_SCOPE_EXIT([&]() { glDeleteProgram(program_id); });

    bool quit = false;
    while (! quit) {
      quit = loop_once(window);
      render(window, program_id, VAO);
    }
    return stlw::make_empty();
  }
};

struct policy
{
  policy() = delete;
  using game_type = boomhs_game;

  DEFINE_STATIC_WRAPPER_FUNCTION(game_loop, game_type::game_loop);
};

} // ns boomhs
} // ns game
