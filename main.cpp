#include <memory>
#include <SFML/Graphics.hpp>

constexpr unsigned int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.f}, ballSpeed{ 8.f };

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

		void ChangeVelocity(sf::Vector2f& velocity)
		{
			this->velocity = velocity;
		}

		float HorizontalSpeed() const noexcept { return velocity.x; }
		float VerticalSpeed() const noexcept { return velocity.y; }
    };

    struct Drawable : Component
    {
        virtual sf::CircleShape Shape() const = 0;
		virtual void ChangePosition(sf::Vector2f& position) = 0;
		virtual void Move(sf::Vector2f& velocity) = 0;
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

		void ChangePosition(sf::Vector2f& position) override
        {
			shape.setPosition(position);
        }

		void Move(sf::Vector2f& velocity) override
		{
			shape.move(velocity);
		}
    };

    struct Node
    {
    };

    struct Render : Node
    {
        std::shared_ptr<Drawable> drawable;
        std::shared_ptr<Position> position;

		void Update() const
		{
			drawable->ChangePosition(position->position);
		}

		sf::CircleShape Shape() const
		{
			return this->drawable->Shape();
		}
    };

    struct Move : Node
    {
		std::shared_ptr<Drawable> drawable;
        std::shared_ptr<Velocity> velocity;
    };

    struct System
    {
    };

    struct Mover : System
    {
        std::shared_ptr<Move> move;

		void Update() const
		{
			move->drawable->Move(sf::Vector2f{move->velocity->HorizontalSpeed(), move->velocity->VerticalSpeed()});
		}
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
	        window.draw(render->Shape());
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

	struct MoverFactory
	{
		static std::shared_ptr<Mover> Create(const std::shared_ptr<Drawable>& drawable, std::shared_ptr<Velocity>& velocity)
		{
			auto move = std::make_shared<Arkanoid::Move>();
			move->drawable = drawable;
			move->velocity = velocity;

			auto mover = std::make_shared<Arkanoid::Mover>();
			mover->move = move;

			return mover;
		}
	};

	struct RendererFactory
	{
		static std::shared_ptr<Renderer> Create(const std::shared_ptr<Drawable>& drawable, std::shared_ptr<Position>& position, sf::RenderWindow& window)
		{
			auto render = std::make_shared<Arkanoid::Render>();
			render->drawable = drawable;
			render->position = position;

			auto renderer = std::make_shared<Arkanoid::Renderer>(window);
			renderer->render = render;

			render->Update();

			return renderer;
		}
	};
}

int main()
{
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid"};
	sf::Vector2f ballPosition{ windowWidth / 2, windowHeight / 2 };
	sf::Vector2f ballVelocity{ -ballSpeed, -ballSpeed };

	auto circle = std::make_shared<Arkanoid::Circle>(ballRadius, sf::Color::Red);
	auto position = std::make_shared<Arkanoid::Position>(ballPosition);
	auto velocity = std::make_shared<Arkanoid::Velocity>(ballVelocity);

	auto renderer = Arkanoid::RendererFactory::Create(circle, position, window);
	auto mover = Arkanoid::MoverFactory::Create(circle, velocity);

	// Entities
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

		mover->Update();
        renderer->Update(window);
    }

    return 0;
}