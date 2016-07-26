#pragma once
#include <string>
#include <iomanip>
#include <boost/expected/expected.hpp>

#include <game/Debug.hpp>
#include <game/Globals.hpp>

#include <engine/gfx/glew_gfx.hpp>
#include <engine/window/sdl_window.hpp>

#include <engine/gfx/Shader.hpp>


// TODO: rm
SDL_GLContext gl_context;
auto const print_matrix = [](auto const& matrix)
  {
    for (auto i = 0; i < 16; ++i) {
      if ((i > 0) && ((i % 4) == 0)) {
        std::cerr << "\n";
      }
      std::cerr << " " << "[" << std::to_string(matrix[i]) << "]";
    }
    std::cerr << "\n" << std::endl;
  };

namespace game
{
namespace sdl
{

boost::expected<GlobalsInitOk, std::string>
init()
{
  // (from the docs) The requested attributes should be set before creating an OpenGL window
  auto const set_attribute = [](auto const attribute, auto const value)
  {
    bool const set_attribute_suceeded = SDL_GL_SetAttribute(attribute, value);
    if (! set_attribute_suceeded) {
      std::cerr << "Setting attribute '" << std::to_string(attribute) << "' failed, error is '"
        << SDL_GetError() << "'\n";
    }
  };

  //Use OpenGL 3.1 core
  set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  set_attribute(SDL_GL_DOUBLEBUFFER, 1);

  // Use v-sync
  SDL_GL_SetSwapInterval(1);

  // testing, remove??
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Initialize video subsystem
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    // Display error message
    auto const fmt = boost::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return FORMAT_STRERR(fmt);
  }
  return GlobalsInitOk{};
}

void
destroy()
{
  if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
    SDL_Quit();
  }
}

using WindowType = SDL_Window;
using WindowPtr = std::unique_ptr<WindowType, decltype(&SDL_DestroyWindow)>;
class Window
{
  WindowPtr w_;
public:
  // ctors
  Window(WindowPtr &&w) : w_(std::move(w)) {}

  // movable, not copyable
  Window(Window &&) = default;
  Window& operator=(Window &&) = default;

  Window(Window const&) = delete;
  Window& operator=(Window const&) = delete;

  // Allow getting the window's SDL pointer
  WindowType* raw() { return this->w_.get(); }
};

boost::expected<Window, std::string>
make_window()
{
  // Hidden dependency between the ordering here, so all the logic exists in one place.
  //
  // * The OpenGL context MUST be initialized before the call to glewInit() takes place.
  // This is because there is a hidden dependency on glew, it expects an OpenGL context to be
  // initialized. The glew library knows how to find the OpenGL context in memory without any
  // reference, so it's a bit like magic.
  //
  // NOTE: We don't have to do anything to shutdown glew, the processing closing will handle it (by
  // design.

  // First, create the SDL window.
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto const height = 800, width = 600;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
  if (nullptr == raw) {
    auto const fmt = boost::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return boost::make_unexpected(boost::str(fmt));
  }
  WindowPtr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  gl_context = SDL_GL_CreateContext(window_ptr.get());
  if(nullptr == gl_context) {
    // Display error message
    auto const fmt = boost::format("OpenGL context could not be created! SDL Error: %s\n")
      % SDL_GetError();
    return FORMAT_STRERR(fmt);
  }
  SDL_GL_MakeCurrent(window_ptr.get(), gl_context);

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const fmt = boost::format("GLEW could not initialize! GLEW error: %s\n")
      % glewGetErrorString(glew_status);
    return FORMAT_STRERR(fmt);
  }
  return Window{std::move(window_ptr)};
}

void
game_loop(Window &&window)
{
  bool quit = false;
  SDL_Event sdlEvent;

  // Create and compile our GLSL program from the shaders
  auto const expected_program = engine::gfx::LoadShaders("shader.vert", "shader.frag");
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
}

} // ns sdl
} // ns game
