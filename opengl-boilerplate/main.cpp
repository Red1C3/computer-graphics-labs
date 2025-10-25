#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

int main()
{
    sf::Window window(sf::VideoMode({ 800, 600 }), "OpenGL", sf::Style::Default, sf::State::Windowed);

    bool running = true;
    while (running)
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                running = false;
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                glViewport(0, 0, resized->size.x, resized->size.y);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        window.display();
    }

}