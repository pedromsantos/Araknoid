#include <memory>
#include <SFML/Graphics.hpp>

constexpr float paddleWidth{ 60.f }, paddleHeight{ 20.f }, paddleVelocity{ 6.f };
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

		void Left()
		{
			this->velocity.x = -ballSpeed;
		}

		void Right()
		{
			this->velocity.x = ballSpeed;
		}

		void Forward()
		{
			this->velocity.y = -ballSpeed;
		}

		void Backward()
		{
			this->velocity.y = ballSpeed;
		}

		float HorizontalSpeed() const noexcept { return velocity.x; }
		float VerticalSpeed() const noexcept { return velocity.y; }
    };

	struct Element : Component
	{
		virtual void ChangePosition(sf::Vector2f& position) = 0;
		virtual void Draw(sf::RenderWindow& window) const noexcept = 0;
	};

	template<class T>
    struct Drawable : Element
    {
		T shape;

		T Shape() const
		{
			return this->shape;
		}

		void Draw(sf::RenderWindow& window) const noexcept override
		{
			window.draw(shape);
		}

		void ChangePosition(sf::Vector2f& position) override
		{
			shape.setPosition(position);
		}

		void Move(sf::Vector2f& velocity)
		{
			shape.move(velocity);
		}

		virtual float x() const noexcept = 0;
		virtual float y() const noexcept = 0;
		virtual float left() const noexcept = 0;
		virtual float right() const noexcept = 0;
		virtual float top() const noexcept = 0;
		virtual float bottom() const noexcept = 0;
    };

    struct Circle : Drawable<sf::CircleShape>
    {
        Circle(float radius, sf::Color color)
        {
            shape.setRadius(radius);
            shape.setFillColor(color);
            shape.setOrigin(radius, radius);
        }

		float x() const noexcept override { return shape.getPosition().x; }
		float y() const noexcept override { return shape.getPosition().y; }
		float left() const noexcept override { return x() - shape.getRadius(); }
		float right() const noexcept override { return x() + shape.getRadius(); }
		float top() const noexcept override { return y() - shape.getRadius(); }
		float bottom() const noexcept override { return y() + shape.getRadius(); }
    };
	
	struct Rectangle : Drawable<sf::RectangleShape>
	{
		Rectangle(float width, float height, sf::Color color)
		{
			shape.setSize(sf::Vector2f(width, height));
			shape.setFillColor(color);
			shape.setOrigin(width / 2.f, height / 2.f);
		}

		float x() const noexcept override { return shape.getPosition().x; }
		float y() const noexcept override { return shape.getPosition().y; }
		float left() const noexcept override { return x() - shape.getSize().x / 2.f; }
		float right() const noexcept override { return x() + shape.getSize().x / 2.f; }
		float top() const noexcept override { return y() - shape.getSize().y / 2.f; }
		float bottom() const noexcept override { return y() + shape.getSize().y / 2.f; }
	};

    struct Node
    {
    };

    struct Render : Node
    {
        std::shared_ptr<Element> drawable;
        std::shared_ptr<Position> position;

		void Update() const
		{
			drawable->ChangePosition(position->position);
		}

		void Draw(sf::RenderWindow& window) const
		{
			drawable->Draw(window);
		}
    };

	template<class T>
    struct Move : Node
    {
		std::shared_ptr<Drawable<T>> drawable;
        std::shared_ptr<Velocity> velocity;

		void Update() const
		{
			drawable->Move(velocity->velocity);
		}

		float x() const noexcept { return drawable->x(); }
		float y() const noexcept { return drawable->y(); }
		float left() const noexcept { return drawable->left(); }
		float right() const noexcept { return drawable->right(); }
		float top() const noexcept { return drawable->top(); }
		float bottom() const noexcept { return drawable->bottom(); }
    };

    struct System
    {
	    virtual ~System()
	    {
	    }
    };

	template<class T>
    struct Mover : System
    {
        std::shared_ptr<Move<T>> move;

		virtual void Update() const
		{
			auto velocity = sf::Vector2f{move->velocity->HorizontalSpeed(), move->velocity->VerticalSpeed()};
			move->drawable->Move(velocity);
		}
    };

	template<class T>
	struct BounceOnEdgesMover : Mover<T>
	{
		void Update() const override
		{
			if (move->left() < 0)
			{
				move->velocity->Right();
			}
			else if (move->right() > windowWidth)
			{
				move->velocity->Left();
			}

			if (move->top() < 0)
			{
				move->velocity->Backward();
			}
			else if (move->bottom() > windowHeight)
			{
				move->velocity->Forward();
			}

			move->Update();
		}
	};

    struct Renderer : System
    {
        std::vector<std::shared_ptr<Render>> renders;

        Renderer(sf::RenderWindow& window)
        {
            window.setFramerateLimit(60);
        }

        void Update(sf::RenderWindow& window) const
        {
            window.clear(sf::Color::Black);

			for (auto render : renders)
			{
				render->Draw(window);
			}

        	window.display();
        }

	    void Add(const std::shared_ptr<Render>& render)
        {
			renders.push_back(render);
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

	template<class T>
	struct MoverFactory
	{
		static std::shared_ptr<Mover<T>> Create(std::shared_ptr<Drawable<T>> drawable, std::shared_ptr<Velocity>& velocity)
		{
			auto move = std::make_shared<Arkanoid::Move<T>>();
			move->drawable = drawable;
			move->velocity = velocity;

			auto mover = std::make_shared<Arkanoid::BounceOnEdgesMover<T>>();
			mover->move = move;

			return mover;
		}
	};

	struct RendererBuilder
	{
		std::shared_ptr<Arkanoid::Renderer> renderer;

		RendererBuilder(sf::RenderWindow& window)
		{
			renderer = std::make_shared<Arkanoid::Renderer>(window);
		}

		RendererBuilder* AddRender(std::shared_ptr<Element> drawable, std::shared_ptr<Position>& position)
		{
			auto render = std::make_shared<Arkanoid::Render>();
			render->drawable = drawable;
			render->position = position;

			render->Update();
			renderer->Add(render);

			return this;
		}

		std::shared_ptr<Arkanoid::Renderer> Create() const
		{
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
	
	sf::Vector2f paddleDefaultpPosition{ windowWidth / 2, windowHeight - 50 };

	auto rectangle = std::make_shared<Arkanoid::Rectangle>(paddleWidth, paddleHeight, sf::Color::Green);
	auto paddlePosition = std::make_shared<Arkanoid::Position>(paddleDefaultpPosition);

	auto rendererBuilder = Arkanoid::RendererBuilder(window);
	rendererBuilder.AddRender(circle, position);
	rendererBuilder.AddRender(rectangle, paddlePosition);

	auto renderer = rendererBuilder.Create();

	auto ballMover = Arkanoid::MoverFactory<sf::CircleShape>::Create(circle, velocity);

	//auto ball = Arkanoid::Ball();
	//ball.position = position;
	//ball.circle = circle;

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

		ballMover->Update();
        renderer->Update(window);
    }

    return 0;
}