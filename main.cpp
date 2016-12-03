#include <memory>
#include <SFML/Graphics.hpp>

constexpr float paddleWidth{ 60.f }, paddleHeight{ 20.f }, paddleSpeed{ 6.f };
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
	    float speed;

	    Velocity(const sf::Vector2f& velocity, float speed) : velocity{velocity}, speed(speed) {}

		void ChangeVelocity(sf::Vector2f& velocity)
		{
			this->velocity = velocity;
		}

		void Left()
		{
			this->velocity.x = -speed;
		}

		void Right()
		{
			this->velocity.x = speed;
		}

		void Forward()
		{
			this->velocity.y = -speed;
		}

		void Backward()
		{
			this->velocity.y = speed;
		}

		void Stop()
		{
			this->velocity.x = 0;
			this->velocity.y = 0;
		}

		void Accelerate(float speed)
		{
			this->speed = speed;
		}

		float HorizontalSpeed() const noexcept { return velocity.x; }
		float VerticalSpeed() const noexcept { return velocity.y; }
    };

	struct Body : Component
	{
		virtual void ChangePosition(sf::Vector2f& position) = 0;
		virtual void Draw(sf::RenderWindow& window) const noexcept = 0;

		virtual float x() const noexcept = 0;
		virtual float y() const noexcept = 0;
		virtual float left() const noexcept = 0;
		virtual float right() const noexcept = 0;
		virtual float top() const noexcept = 0;
		virtual float bottom() const noexcept = 0;
	};

	template<class T>
    struct Drawable : Body
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
	    virtual ~Node()
	    {
	    }
    };

    struct Render : Node
    {
        std::shared_ptr<Body> drawable;
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

		virtual void Left()
		{
			this->velocity->Left();
		}

		virtual void Right()
		{
			this->velocity->Right();
		}

		virtual void Forward()
		{
			this->velocity->Forward();
		}

		virtual void Backward()
		{
			this->velocity->Backward();
		}

		virtual void Stop()
		{
			this->velocity->Stop();
		}

		void Accelerate(float speed)
		{
			this->velocity->Accelerate(speed);
		}

		float x() const noexcept { return drawable->x(); }
		float y() const noexcept { return drawable->y(); }
		float left() const noexcept { return drawable->left(); }
		float right() const noexcept { return drawable->right(); }
		float top() const noexcept { return drawable->top(); }
		float bottom() const noexcept { return drawable->bottom(); }
    };

	template<class T>
	struct BoundedMove : Move<T>
	{
		int top, bottom, left, right;

		BoundedMove(int top, int bottom, int left, int right)
		{
			this->top = top;
			this->bottom = bottom;
			this->left = left;
			this->right = right;
		}

		void Left() override
		{
			if (Move<T>::left() > left)
			{
				Move<T>::Left();
			}
		}

		void Right() override
		{
			if (Move<T>::right() < right)
			{
				Move<T>::Right();
			}
		}

		void Forward() override
		{
			if (Move<T>::top() > top)
			{
				Move<T>::Forward();
			}
		}

		void Backward() override
		{
			if (Move<T>::bottom() < bottom)
			{
				Move<T>::Backward();
			}
		}
	};

    struct System
    {
		virtual void Update() const {};

	    virtual ~System()
	    {
	    }
    };

	template<class T>
    struct Mover : System
    {
        std::shared_ptr<Move<T>> move;

	    void Update() const override
		{
			auto velocity = sf::Vector2f{move->velocity->HorizontalSpeed(), move->velocity->VerticalSpeed()};
			move->drawable->Move(velocity);
		}
    };

	template<class T>
	struct MoveOnLeftRightKey : System
	{
		std::shared_ptr<BoundedMove<T>> move;
		float speed;

		MoveOnLeftRightKey(float speed): speed(speed) 
		{	
		}

		void Update() const override
		{
			move->Stop();

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
			{
				move->Accelerate(speed);
				move->Left();
			}
			
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
			{
				move->Accelerate(speed);
				move->Right();
			}

			this->move->Update();

			move->Stop();
		}
	};

	template<class T>
	struct BounceOnEdgesMover : Mover<T>
	{
		void Update() const override
		{
			if (this->move->left() < 0)
			{
                this->move->Right();
			}
			else if (this->move->right() > windowWidth)
			{
                this->move->Left();
			}

			if (this->move->top() < 0)
			{
                this->move->Backward();
			}
			else if (this->move->bottom() > windowHeight)
			{
                this->move->Forward();
			}

            this->move->Update();
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

	struct Intent : System
	{
		
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
            velocity = std::make_shared<Velocity>(ballVelocity, ballSpeed);
        }
    };

	template<class T>
	struct MoverFactory
	{
		static std::shared_ptr<System> Create(std::shared_ptr<Drawable<T>> drawable, std::shared_ptr<Velocity>& velocity)
		{
			auto move = std::make_shared<Arkanoid::Move<T>>();
			move->drawable = drawable;
			move->velocity = velocity;

			auto mover = std::make_shared<Arkanoid::BounceOnEdgesMover<T>>();
			mover->move = move;

			return mover;
		}

		static std::shared_ptr<System> CreateKeyBounded(std::shared_ptr<Drawable<T>> drawable, std::shared_ptr<Velocity>& velocity, float speed)
		{
			auto move = std::make_shared<Arkanoid::BoundedMove<T>>(0, windowHeight, 0, windowWidth);
			move->drawable = drawable;
			move->velocity = velocity;

			auto mover = std::make_shared<Arkanoid::MoveOnLeftRightKey<T>>(speed);
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

		RendererBuilder* AddRender(std::shared_ptr<Body> drawable, std::shared_ptr<Position>& position)
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

	struct BallFactory
	{
		auto static create(Arkanoid::RendererBuilder& rendererBuilder)
		{
			sf::Vector2f ballPosition{ windowWidth / 2, windowHeight / 2 };
			sf::Vector2f ballVelocity{ -ballSpeed, -ballSpeed };

			auto circle = std::make_shared<Arkanoid::Circle>(ballRadius, sf::Color::Red);
			auto position = std::make_shared<Arkanoid::Position>(ballPosition);
			auto velocity = std::make_shared<Arkanoid::Velocity>(ballVelocity, ballSpeed);

			rendererBuilder.AddRender(circle, position);

            return MoverFactory<sf::CircleShape>::Create(circle, velocity);
		}
	};

	struct PaddleFactory
	{
		auto static create(Arkanoid::RendererBuilder& rendererBuilder)
		{
			sf::Vector2f paddleDefaultVelocity{ 0, 0 };
			sf::Vector2f paddleDefaultPosition{ windowWidth / 2, windowHeight - 50 };

			auto rectangle = std::make_shared<Arkanoid::Rectangle>(paddleWidth, paddleHeight, sf::Color::Green);
			auto paddlePosition = std::make_shared<Arkanoid::Position>(paddleDefaultPosition);
			auto paddleVelocity = std::make_shared<Arkanoid::Velocity>(paddleDefaultVelocity, paddleSpeed);

			rendererBuilder.AddRender(rectangle, paddlePosition);

            return MoverFactory<sf::RectangleShape>::CreateKeyBounded(rectangle, paddleVelocity, paddleSpeed);
		}
	};
}

int main()
{
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid"};

	auto rendererBuilder = Arkanoid::RendererBuilder(window);
    auto ballMover = Arkanoid::BallFactory::create(rendererBuilder);
    auto paddleMover = Arkanoid::PaddleFactory::create(rendererBuilder);
	auto renderer = rendererBuilder.Create();

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
		paddleMover->Update();
        renderer->Update(window);
    }

    return 0;
}