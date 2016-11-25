#include <SFML/Graphics.hpp>
#include <cmath>

constexpr int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.f}, ballVelocity{8.f};
constexpr float paddleWidth{60.f}, paddleHeight{20.f}, paddleVelocity{6.f};
constexpr float blockWidth{60.f}, blockHeight{20.f};
constexpr int countBlocksX{11}, countBlocksY{4};

struct Object
{
    virtual const float X() = 0 ;
    virtual const float Y() = 0;
    virtual const float Left() = 0;
    virtual const float Right() = 0;
    virtual const float Top() = 0;
    virtual const float Bottom() = 0;

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
            moveRight();
        }
        else if(Right() > windowWidth)
        {
            moveLeft();
        }

        if(Top() < 0)
        {
            moveDown();
        }
        else if(Bottom() > windowHeight)
        {
            moveUp();
        }
    }

    void moveLeft()
    {
        velocity.x = -ballVelocity;
    }

    void moveRight()
    {
        velocity.x = ballVelocity;
    }

    void moveDown()
    {
        velocity.y = ballVelocity;
    }

    void moveUp()
    {
        velocity.y = -ballVelocity;
    }

    float const X() { return shape.getPosition().x; }
    float const Y() { return shape.getPosition().y; }
    float const Left() { return X() - shape.getRadius(); }
    float const Right() { return X() + shape.getRadius(); }
    float const Top() { return Y() - shape.getRadius(); }
    float const Bottom() { return Y() + shape.getRadius(); }
};

struct Rectangle : Object
{
    sf::RectangleShape shape;

    float const X() { return shape.getPosition().x; }
    float const Y() { return shape.getPosition().y; }
    float const Left() { return X() - shape.getSize().x / 2.f; }
    float const Right() { return X() + shape.getSize().x / 2.f; }
    float const Top() { return Y() - shape.getSize().x / 2.f; }
    float const Bottom() { return Y() + shape.getSize().x / 2.f; }
};

struct Paddle : Rectangle
{
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

    void updateBallDirectionOnCollision(Ball &ball)
    {
        if(!isIntersecting(ball))
        {
            return;
        }

        ball.moveUp();

        if(ball.X() < this->X())
        {
            ball.moveLeft();
        }
        else
        {
            ball.moveRight();
        }
    }
};

struct Block : Rectangle
{
    Block(int x, int y)
    {
        shape.setPosition(x, y);
        shape.setSize({blockWidth, blockHeight});
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    virtual std::unique_ptr<Block> updateBallDirectionOnCollision(Ball &ball);
};

struct DestroyedBlock : Block
{
    DestroyedBlock(int x, int y)
        :Block(x, y)
    {
        shape.setFillColor(sf::Color::Black);
    }

    std::unique_ptr<Block> updateBallDirectionOnCollision(Ball &ball) override
    {
        return std::make_unique<DestroyedBlock>(this->X(), this->Y());
    }
};

std::unique_ptr<Block> Block::updateBallDirectionOnCollision(Ball &ball)
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
        if(ballFromLeft)
        {
            ball.moveLeft();
        }
        else
        {
            ball.moveRight();
        }
    }
    else
    {
        if(ballFromTop)
        {
            ball.moveUp();
        }
        else
        {
            ball.moveDown();
        }
    }

    return  std::make_unique<DestroyedBlock>((int)this->X(), (int)this->Y());
}

class Game
{
private:
    sf::RenderWindow window{{windowWidth, windowHeight}, "Arkanoid - 1"};

    Ball ball{windowWidth / 2, windowHeight / 2};
    Paddle paddle{windowWidth / 2, windowHeight - 50};
    std::vector<std::unique_ptr<Block>> blocks;

public:

    void run()
    {
        window.setFramerateLimit(60);
        createBlocks();

        while(window.isOpen())
        {
            window.clear(sf::Color::Black);

            input();
            update();
            draw();

            window.display();
        }
    }

private:
    void createBlocks()
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

    void input()
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
            }
        }
    }

    void update()
    {
        ball.update();
        paddle.update();

        paddle.updateBallDirectionOnCollision(ball);
        updateBallDirectionOnBlockCollision();
    }

    void draw()
    {
        for(auto& block: blocks)
        {
            window.draw(block->shape);
        }

        window.draw(paddle.shape);
        window.draw(ball.shape);
    }

    void updateBallDirectionOnBlockCollision()
    {
        for (int i = 0; i < blocks.size(); ++i)
        {
            blocks[i] = std::move(blocks[i]->updateBallDirectionOnCollision(ball));
        }
    }
};

int main(int argc, char* argv[]) {

    Game{}.run();

    return 0;
}
