#include <SFML/Graphics.hpp>
#include <cmath>

constexpr int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.f}, ballVelocity{8.f};
constexpr float paddleWidth{60.f}, paddleHeight{20.f}, paddleVelocity{6.f};
constexpr float blockWidth{60.f}, blockHeight{20.f};
constexpr int countBlocksX{11}, countBlocksY{4};

struct Object
{
    virtual float X() = 0;
    virtual float Y() = 0;
    virtual float Left() = 0;
    virtual float Right() = 0;
    virtual float Top() = 0;
    virtual float Bottom() = 0;

    template<class T> bool isIntersecting(T& other)
    {
        return this->Right() >= other.Left() && this->Left() <= other.Right()
           && this->Bottom() >= other.Top() && this->Top() <= other.Bottom();
    };
};

struct Ball : Object
{
    sf::CircleShape shape;
    sf::Vector2f velocity{-ballVelocity, -ballVelocity};

    Ball(float x, float y)
    {
        shape.setPosition(x, y);
        shape.setRadius(ballRadius);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(ballRadius, ballRadius);
    }

    void update()
    {
        shape.move(velocity);

        if(Left() < 0)
        {
            velocity.x = ballVelocity;
        }
        else if(Right() > windowWidth)
        {
            velocity.x = -ballVelocity;
        }

        if(Top() < 0)
        {
            velocity.y = ballVelocity;
        }
        else if(Bottom() > windowHeight)
        {
            velocity.y = -ballVelocity;
        }
    }

    float X() { return shape.getPosition().x; }
    float Y() { return shape.getPosition().y; }
    float Left() { return X() - shape.getRadius(); }
    float Right() { return X() + shape.getRadius(); }
    float Top() { return Y() - shape.getRadius(); }
    float Bottom() { return Y() + shape.getRadius(); }
};

struct Paddle : Object
{
    sf::RectangleShape shape;
    sf::Vector2f velocity;

    Paddle(int x, int y)
    {
        shape.setPosition(x, y);
        shape.setSize({paddleWidth, paddleHeight});
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    void update()
    {
        shape.move(velocity);

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)
                && Left() > 0)
        {
            velocity.x = -paddleVelocity;
        }
        else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
           && Right() < windowWidth)
        {
            velocity.x = paddleVelocity;
        }
        else
        {
            velocity.x = 0;
        }
    }

    void collidingWith(Ball &ball)
    {
        if(!isIntersecting(ball))
        {
            return;
        }

        ball.velocity.y = -ballVelocity;

        if(ball.X() < this->X())
        {
            ball.velocity.x = -ballVelocity;
        }
        else
        {
            ball.velocity.x = ballVelocity;
        }
    }

    float X() { return shape.getPosition().x; }
    float Y() { return shape.getPosition().y; }
    float Left() { return X() - shape.getSize().x / 2.f; }
    float Right() { return X() + shape.getSize().x / 2.f; }
    float Top() { return Y() - shape.getSize().x / 2.f; }
    float Bottom() { return Y() + shape.getSize().x / 2.f; }
};

struct Block : Object
{
    sf::RectangleShape shape;

    Block(int x, int y)
    {
        shape.setPosition(x, y);
        shape.setSize({blockWidth, blockHeight});
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    float X() { return shape.getPosition().x; }
    float Y() { return shape.getPosition().y; }
    float Left() { return X() - shape.getSize().x / 2.f; }
    float Right() { return X() + shape.getSize().x / 2.f; }
    float Top() { return Y() - shape.getSize().x / 2.f; }
    float Bottom() { return Y() + shape.getSize().x / 2.f; }

    virtual std::unique_ptr<Block> collidingWith(Ball &ball);
};

struct DestroyedBlock : Block
{
    DestroyedBlock(int x, int y)
        :Block(x, y)
    {
        shape.setFillColor(sf::Color::Black);
    }

    std::unique_ptr<Block> collidingWith(Ball &ball) override
    {
        return std::make_unique<DestroyedBlock>(this->X(), this->Y());
    }
};

std::unique_ptr<Block> Block::collidingWith(Ball &ball)
{
    if(!isIntersecting(ball))
    {
        return std::make_unique<Block>(this->X(), this->Y());;
    }

    float overlapLeft{ball.Right() - this->Left()};
    float overlapRight{this->Right() - ball.Left()};
    float overlapTop{ball.Top() - ball.Bottom()};
    float overlapBottom{this->Bottom() - ball.Top()};

    bool ballFromLeft{std::abs(overlapLeft) < std::abs(overlapRight)};
    bool ballFromTop{std::abs(overlapTop) < std::abs(overlapBottom)};
    float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
    float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

    if(std::abs(minOverlapX) < std::abs(minOverlapY))
    {
        ball.velocity.x = ballFromLeft ? -ballVelocity : ballVelocity;
    }
    else
    {
        ball.velocity.y = ballFromTop ? -ballVelocity : ballVelocity;
    }

    return  std::make_unique<DestroyedBlock>((int)this->X(), (int)this->Y());
}

void initializeBlocks(std::vector<std::unique_ptr<Block>> &blocks);

void handleBlockCollisions(Ball &ball, std::vector<std::unique_ptr<Block>> &blocks);

void redrawObjects(sf::RenderWindow &window, const Ball &ball, const Paddle &paddle,
                   const std::vector<std::unique_ptr<Block>> &blocks);

int main(int argc, char* argv[]) {
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid - 1"};
    window.setFramerateLimit(60);

    Ball ball{windowWidth / 2, windowHeight / 2};
    Paddle paddle{windowWidth / 2, windowHeight - 50};
    std::vector<std::unique_ptr<Block>> blocks;

    initializeBlocks(blocks);

    while(window.isOpen())
    {
        sf::Event Event;

        while (window.pollEvent(Event))
        {
            if (Event.type == sf::Event::Closed)
            {
                window.close();
            }

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
            {
                window.close();
           }
        }

        window.clear(sf::Color::Black);

        ball.update();
        paddle.update();

        paddle.collidingWith(ball);
        handleBlockCollisions(ball, blocks);

        redrawObjects(window, ball, paddle, blocks);

        window.display();
    }

    return 0;
}

void redrawObjects(sf::RenderWindow &window, const Ball &ball, const Paddle &paddle,
                   const std::vector<std::unique_ptr<Block>> &blocks)
{
    for(auto& block: blocks)
    {
        window.draw(block->shape);
    }

    window.draw(paddle.shape);
    window.draw(ball.shape);
}

void handleBlockCollisions(Ball &ball, std::vector<std::unique_ptr<Block>> &blocks)
{
    for (int i = 0; i < blocks.size(); ++i)
    {
        blocks[i] = std::move(blocks[i]->collidingWith(ball));
    }
}

void initializeBlocks(std::vector<std::unique_ptr<Block>> &blocks)
{
    for (int x{0}; x < countBlocksX; ++x)
    {
        for (int y{0}; y < countBlocksY; ++y)
        {
            auto block = std::make_unique<Block>((x + 1) * (blockWidth + 3) + 22,
                                (y + 2) * (blockHeight + 3));

            blocks.push_back(move(block));
        }
    }
}