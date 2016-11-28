#include <memory>
#include <SFML/Graphics.hpp>

constexpr unsigned int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.f};

namespace Arkanoid
{
    struct Component
    {
	    virtual ~Component()
	    {
	    }
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

    struct Drawable : Component
    {
        virtual sf::CircleShape Shape() const = 0;
		virtual void ChangePosition(sf::Vector2f position) = 0;
    };

    struct Circle : Drawable
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

		void ChangePosition(sf::Vector2f position) override
        {
			shape.setPosition(position);
        }
    };

    struct Node
    {
    };

    struct Render : Node
    {
        std::shared_ptr<Drawable> display;
        std::shared_ptr<Position> position;

		void Update() const
		{
			display->ChangePosition(position->position);
		}
    };

    struct Move : Node
    {
        std::shared_ptr<Position> position;
        std::shared_ptr<Velocity> velocity;
    };

    struct System
    {
    };

    struct Mover : System
    {
        std::shared_ptr<Move> move;
    };

    struct Renderer : System
    {
        std::shared_ptr<Render> render;

        Renderer(sf::RenderWindow& window)
        {
            window.setFramerateLimit(60);
        }

        void Update(sf::RenderWindow& window) const
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
        std::shared_ptr<Circle> circle;
        std::shared_ptr<Position> position;
        std::shared_ptr<Velocity> velocity;

        Ball()
        {
            sf::Vector2f ballVelocity{0, 0};
            velocity = std::make_shared<Velocity>(ballVelocity);
        }
    };
}

int main()
{
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid"};
	sf::Vector2f ballPosition{ windowWidth / 2, windowHeight / 2 };

	auto circle = std::make_shared<Arkanoid::Circle>(ballRadius, sf::Color::Red);
	auto position = std::make_shared<Arkanoid::Position>(ballPosition);

	auto render = std::make_shared<Arkanoid::Render>();
	render->display = circle;
	render->position = position;

	auto renderer = Arkanoid::Renderer(window);
	renderer.render = render;

	auto ball = Arkanoid::Ball();
	ball.position = position;
	ball.circle = circle;

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

		render->Update();
        renderer.Update(window);
    }

    return 0;
}