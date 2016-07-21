#include <memory>

// OpenGL headers
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

// SDL headers
#include <SDL_main.h>
#include <SDL.h>
#include <SDL_opengl.h>

// Boost stuff
#include <boost/expected/expected.hpp>
#include <boost/format.hpp>

bool quit;

SDL_GLContext glContext;
SDL_Event sdlEvent;

auto
make_thing() { return boost::make_unexpected("hi"); }

auto
make_window()
{
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto const height = 800, width = 600;
  auto raw = SDL_CreateWindow(title, x, y, width, height, flags);

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

  if (nullptr == raw) {
    auto const fmt = boost::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return boost::make_unexpected(boost::str(fmt));
  }
  WindowPtr window_ptr{raw, &SDL_DestroyWindow};
  return boost::make_expected(Window{std::move(window_ptr)});
}

int
main(int argc, char *argv[])
{
    quit = false;

    //Use OpenGL 3.1 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Initialize video subsystem
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        // Display error message
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    auto window = make_window();
    if(window.raw() == NULL )
    {
        // Display error message
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        return false;
    }
    else
    {
        // Create OpenGL context
        glContext = SDL_GL_CreateContext(window.raw());

        if( glContext == NULL )
        {
            // Display error message
            printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
            return false;
        }
        else
        {
            // Initialize glew
            glewInit();
        }
    }

    // Game loop
    while (!quit)
    {
        while(SDL_PollEvent(&sdlEvent) != 0)
        {
            // Esc button is pressed
            if(sdlEvent.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        // Set background color as cornflower blue
        glClearColor(0.39f, 0.58f, 0.93f, 1.f);
        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Update window with OpenGL rendering
        SDL_GL_SwapWindow(window.raw());
    }

    //Quit SDL subsystems
    SDL_Quit();
    return 0;
}
