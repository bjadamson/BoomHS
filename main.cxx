#include <iostream>
#include <SFML/Graphics.hpp>

int notmain()
{
  sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
  sf::CircleShape shape(100.f);
  shape.setFillColor(sf::Color::Green);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      window.clear();
      window.draw(shape);
      window.display();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
      return 0;
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

