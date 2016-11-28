#include <memory>
#include <SFML/Graphics.hpp>

constexpr unsigned int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.f};

namespace Arkanoid
{
    struct Component
    {
    };

    struct Position : Component
    {
        sf::Vector2f position;

        Position(const sf::Vector2f& position) : position{position} {}

        float X() const noexcept { return position.x; }
        float Y() const noexcept { return position.y; }
    };

    struct Velocity : Component
    {
        sf::Vector2f velocity;

        Velocity(const sf::Vector2f& velocity) : velocity{velocity} {}
    };

    struct Dispaly : Component
    {
        virtual sf::CircleShape Shape() const = 0;
    };

    struct Circle : Dispaly
    {
        sf::CircleShape shape;

        Circle(float radius, sf::Color color)
        {
            shape.setRadius(radius);
            shape.setFillColor(color);
            shape.setOrigin(radius, radius);
        }

        sf::CircleShape Shape() const override
        {
            return this->shape;
        }
    };

    struct Node
    {
    };

    struct Render : Node
    {
        std::unique_ptr<Dispaly> display;
        std::unique_ptr<Position> position;
    };

    struct Move : Node
    {
        std::unique_ptr<Position> position;
        std::unique_ptr<Velocity> velocity;
    };

    struct System
    {
    };

    struct Mover : System
    {
        std::unique_ptr<Move> move;
    };

    struct Renderer : System
    {
        std::unique_ptr<Render> render;

        Renderer(sf::RenderWindow& window)
        {
            window.setFramerateLimit(60);
        }

        void Update(sf::RenderWindow& window)
        {
            window.clear(sf::Color::Black);
            auto shape = render->display->Shape();

            window.draw(shape);
            window.display();
        }
    };

    struct Entity
    {
    };

    struct Ball : Entity
    {
        std::unique_ptr<Circle> circle;
        std::unique_ptr<Position> position;
        std::unique_ptr<Velocity> velocity;

        Ball()
        {
            sf::Vector2f ballVelocity{0, 0};
            sf::Vector2f ballPosition{windowWidth / 2, windowHeight / 2};

            circle = std::make_unique<Circle>(ballRadius, sf::Color::Red);
            position = std::make_unique<Position>(ballPosition);

            velocity = std::make_unique<Velocity>(ballVelocity);
        }
    };
}

int main()
{
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid"};

    Arkanoid::Ball ball = Arkanoid::Ball();
    Arkanoid::Renderer renderer = Arkanoid::Renderer(window);

    while(window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
            {
                window.close();
                break;
            }

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
            {
                window.close();
                break;
            }
        }

        renderer.Update(window);
    }

    return 0;
}