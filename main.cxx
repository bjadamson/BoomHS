#include <iostream>
#include <random>
#include <vector>
#include <SFML/Graphics.hpp>
#include <boost/range/irange.hpp>

// OpenGL headers
//#define GLEW_STATIC
//#include <GL/glew.h>
//#include <GL/glu.h>
//#include <GL/gl.h>

// SDL headers
//#include <SDL.h>
//#include <SDL_opengl.h>

void
debug(char const* msg)
{
  std::cerr << msg << std::endl;
}

struct Vertice
{
  int const x, y, z;
  Vertice(int const xp, int const yp, int const zp) : x(xp), y(yp), z(zp) {}
};

template<template<typename...> class C>
struct Terrain
{
  C<Vertice> const vertices;
  Terrain(C<Vertice> &&v) : vertices(std::move(v)) {}
};

// make verice random
auto
make_vertice_r(std::default_random_engine &generator)
{
  std::uniform_int_distribution<int> xd{0, 800}, yd{0, 600}, zd{0, 0};
  return Vertice{xd(generator), yd(generator), zd(generator)};
}

auto
make_terrain(std::default_random_engine &generator)
{
  auto constexpr SIZE = 500;
  std::vector<Vertice> vertices;
  vertices.reserve(SIZE);
  auto constexpr last = 10;
  for(auto i = 0; i < SIZE; ++i) {
    vertices.emplace_back(make_vertice_r(generator));
  }
  return Terrain<std::vector>{std::move(vertices)};
}

template<typename Player>
inline void
move_player(Player &p)
{
  auto constexpr DELTA = 1.01;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    p.move(-DELTA, 0.00);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    p.move(+DELTA, 0.00);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    p.move(0, -DELTA);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    p.move(0, DELTA);
  }
}

template<typename E>
inline bool
should_close(E &event)
{
  bool const window_closed = event.type == sf::Event::Closed;
  bool const escape_key_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Escape);
  return escape_key_pressed || window_closed;
}

template<typename T, typename W>
void
render_terrain(T &terrain, W &window)
{
  std::vector<sf::CircleShape> ccs;
  ccs.reserve(terrain.vertices.size());
  for (auto it : terrain.vertices) {
    sf::CircleShape dot{5.0};
    dot.setPosition(it.x, it.y);
    dot.setFillColor(sf::Color::Blue);
    ccs.emplace_back(dot);
  }
  for (auto it : ccs) {
    window.draw(it);
  }
}

int
notmain()
{
  debug("before");
  sf::RenderWindow window(sf::VideoMode(800, 600), "KILL EM ALL BR");
  debug("after");
  sf::CircleShape minimap{100.f};
  minimap.setFillColor(sf::Color::Green);

  sf::CircleShape player{10.f};
  player.setFillColor(sf::Color::Red);
  player.setPosition(20, 20);

  std::default_random_engine generator;
  auto const terrain = make_terrain(generator);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (should_close(event)) {
        return 0;
      }
      move_player(player);
      window.clear();
      window.draw(minimap);
      window.draw(player);
      render_terrain(terrain, window);
      window.display();
    }
  }
  return 0;
}

int
main(int argc, char **argv)
{
  std::cerr << "saa" << std::endl;
  return notmain();
}

