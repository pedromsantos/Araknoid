#include <iostream>
#include <vector>
#include <array>
#include <cassert>
#include <cmath>
#include <SFML/Graphics.hpp>

namespace CompositionArkanoid
{
    struct Component;
    class Entity;
    class Manager;
    class Game;

    using ComponentID = std::size_t;
    using Group = std::size_t;

    namespace Internal
    {
        inline ComponentID getUniqueComponentID() noexcept
        {
            static ComponentID lastID{0u};
            return lastID++;
        }
    }

    template <typename T>
    inline ComponentID getComponentTypeID() noexcept
    {
        static_assert(std::is_base_of<Component, T>::value,
                      "T must inherit from Component");

        static ComponentID typeID{Internal::getUniqueComponentID()};
        return typeID;
    }

    constexpr std::size_t maxComponents{32};
    using ComponentBitset = std::bitset<maxComponents>;
    using ComponentArray = std::array<Component*, maxComponents>;

    constexpr std::size_t maxGroups{32};
    using GroupBitset = std::bitset<maxGroups>;

    struct Component
    {
        Entity* entity;

        virtual void init() {}
        virtual void update(float mFT) {}
        virtual void draw(Game* game) {}

        virtual ~Component() {}
    };

    class Entity
    {
    private:
        Manager& manager;

        bool alive{true};
        std::vector<std::unique_ptr<Component>> components;
        ComponentArray componentArray;
        ComponentBitset componentBitset;

        GroupBitset groupBitset;

    public:
        Entity(Manager& mManager) : manager(mManager) {}

        void update(float mFT)
        {
            for(auto& c : components) c->update(mFT);
        }
        void draw(Game* game)
        {
            for(auto& c : components) c->draw(game);
        }

        bool isAlive() const { return alive; }
        void destroy() { alive = false; }

        template <typename T>
        bool hasComponent() const
        {
            return componentBitset[getComponentTypeID<T>()];
        }

        bool hasGroup(Group mGroup) const noexcept
        {
            return groupBitset[mGroup];
        }

        void addGroup(Group mGroup) noexcept;
        void delGroup(Group mGroup) noexcept { groupBitset[mGroup] = false; }

        template <typename T, typename... TArgs>
        T& addComponent(TArgs&&... mArgs)
        {
            assert(!hasComponent<T>());

            T* c(new T(std::forward<TArgs>(mArgs)...));
            c->entity = this;
            std::unique_ptr<Component> uPtr{c};
            components.emplace_back(std::move(uPtr));

            componentArray[getComponentTypeID<T>()] = c;
            componentBitset[getComponentTypeID<T>()] = true;

            c->init();
            return *c;
        }

        template <typename T>
        T& getComponent() const
        {
            assert(hasComponent<T>());
            auto ptr(componentArray[getComponentTypeID<T>()]);
            return *reinterpret_cast<T*>(ptr);
        }
    };

    struct Manager
    {
    private:
        std::vector<std::unique_ptr<Entity>> entities;
        std::array<std::vector<Entity*>, maxGroups> groupedEntities;

    public:
        void update(float mFT)
        {
            for(auto& e : entities) e->update(mFT);
        }
        void draw(Game* game)
        {
            for(auto& e : entities) e->draw(game);
        }

        void addToGroup(Entity* mEntity, Group mGroup)
        {
            groupedEntities[mGroup].emplace_back(mEntity);
        }

        std::vector<Entity*>& getEntitiesByGroup(Group mGroup)
        {
            return groupedEntities[mGroup];
        }

        void refresh()
        {
            for(auto i(0u); i < maxGroups; ++i)
            {
                auto& v(groupedEntities[i]);

                v.erase(std::remove_if(std::begin(v), std::end(v),
                                       [i](Entity* mEntity)
                                       {
                                           return !mEntity->isAlive() ||
                                                  !mEntity->hasGroup(i);
                                       }),
                        std::end(v));
            }

            entities.erase(
                    std::remove_if(std::begin(entities), std::end(entities),
                                   [](const std::unique_ptr<Entity>& mEntity)
                                   {
                                       return !mEntity->isAlive();
                                   }),
                    std::end(entities));
        }

        void addEntity(std::unique_ptr<Entity>&& entity)
        {
            entities.emplace_back(std::move(entity));
        }
    };

    void Entity::addGroup(Group mGroup) noexcept
    {
        groupBitset[mGroup] = true;
        manager.addToGroup(this, mGroup);
    }

    using namespace std;
    using namespace sf;
    using FrameTime = float;

    constexpr int windowWidth{800}, windowHeight{600};
    constexpr float ballRadius{10.f}, ballVelocity{0.8f};
    constexpr float paddleWidth{60.f}, paddleHeight{20.f}, paddleVelocity{0.6f};
    constexpr float blockWidth{60.f}, blockHeight{20.f};
    constexpr int countBlocksX{11}, countBlocksY{4};
    constexpr float ftStep{1.f}, ftSlice{1.f};

    struct Game;

    struct CPosition : Component
    {
        Vector2f position;

        CPosition() = default;
        CPosition(const Vector2f& mPosition) : position{mPosition} {}

        float x() const noexcept { return position.x; }
        float y() const noexcept { return position.y; }
    };

    struct CPhysics : Component
    {
        CPosition* cPosition{nullptr};
        Vector2f velocity, halfSize;

        std::function<void(const Vector2f&)> onOutOfBounds;

        CPhysics(const Vector2f& mHalfSize) : halfSize{mHalfSize} {}

        void init() override
        {
            cPosition = &entity->getComponent<CPosition>();
        }

        void update(float mFT) override
        {
            cPosition->position += velocity * mFT;

            if(onOutOfBounds == nullptr) return;

            if(left() < 0)
                onOutOfBounds(Vector2f{1.f, 0.f});
            else if(right() > windowWidth)
                onOutOfBounds(Vector2f{-1.f, 0.f});

            if(top() < 0)
                onOutOfBounds(Vector2f{0.f, 1.f});
            else if(bottom() > windowHeight)
                onOutOfBounds(Vector2f{0.f, -1.f});
        }

        float x() const noexcept { return cPosition->x(); }
        float y() const noexcept { return cPosition->y(); }
        float left() const noexcept { return x() - halfSize.x; }
        float right() const noexcept { return x() + halfSize.x; }
        float top() const noexcept { return y() - halfSize.y; }
        float bottom() const noexcept { return y() + halfSize.y; }
    };

    struct CCircle : Component
    {
        CPosition* cPosition{nullptr};
        CircleShape shape;
        float radius;

        CCircle(float mRadius) : radius{mRadius} {}

        void init() override
        {
            cPosition = &entity->getComponent<CPosition>();

            shape.setRadius(radius);
            shape.setFillColor(Color::Red);
            shape.setOrigin(radius, radius);
        }

        void update(float mFT) override
        {
            shape.setPosition(cPosition->position);
        }
        void draw(Game* game) override;
    };

    struct CRectangle : Component
    {
        CPosition* cPosition{nullptr};
        RectangleShape shape;
        Vector2f size;

        CRectangle(const Vector2f& mHalfSize)
                : size{mHalfSize * 2.f}
        {
        }

        void init() override
        {
            cPosition = &entity->getComponent<CPosition>();

            shape.setSize(size);
            shape.setFillColor(Color::Red);
            shape.setOrigin(size.x / 2.f, size.y / 2.f);
        }

        void update(float mFT) override
        {
            shape.setPosition(cPosition->position);
        }
        void draw(Game* game) override;
    };

    struct CPaddleControl : Component
    {
        CPhysics* cPhysics{nullptr};

        void init() override { cPhysics = &entity->getComponent<CPhysics>(); }

        void update(FrameTime mFT) override
        {
            if(Keyboard::isKeyPressed(Keyboard::Key::Left) &&
               cPhysics->left() > 0)
                cPhysics->velocity.x = -paddleVelocity;
            else if(Keyboard::isKeyPressed(Keyboard::Key::Right) &&
                    cPhysics->right() < windowWidth)
                cPhysics->velocity.x = paddleVelocity;
            else
                cPhysics->velocity.x = 0;
        }
    };

    template <class T1, class T2>
    bool isIntersecting(T1& mA, T2& mB) noexcept
    {
        return mA.right() >= mB.left() && mA.left() <= mB.right() &&
               mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
    }

    void testCollisionPB(Entity& mPaddle, Entity& mBall) noexcept
    {
        auto& cpPaddle(mPaddle.getComponent<CPhysics>());
        auto& cpBall(mBall.getComponent<CPhysics>());

        if(!isIntersecting(cpPaddle, cpBall)) return;

        cpBall.velocity.y = -ballVelocity;
        if(cpBall.x() < cpPaddle.x())
            cpBall.velocity.x = -ballVelocity;
        else
            cpBall.velocity.x = ballVelocity;
    }

    void testCollisionBB(Entity& mBrick, Entity& mBall) noexcept
    {
        auto& cpBrick(mBrick.getComponent<CPhysics>());
        auto& cpBall(mBall.getComponent<CPhysics>());

        if(!isIntersecting(cpBrick, cpBall)) return;
        mBrick.destroy();

        float overlapLeft{cpBall.right() - cpBrick.left()};
        float overlapRight{cpBrick.right() - cpBall.left()};
        float overlapTop{cpBall.bottom() - cpBrick.top()};
        float overlapBottom{cpBrick.bottom() - cpBall.top()};

        bool ballFromLeft(std::abs(overlapLeft) < std::abs(overlapRight));
        bool ballFromTop(std::abs(overlapTop) < std::abs(overlapBottom));

        float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
        float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

        if(std::abs(minOverlapX) < std::abs(minOverlapY))
            cpBall.velocity.x = ballFromLeft ? -ballVelocity : ballVelocity;
        else
            cpBall.velocity.y = ballFromTop ? -ballVelocity : ballVelocity;
    }

    struct Game
    {
        enum ArkanoidGroup : std::size_t
        {
            GPaddle,
            GBrick,
            GBall
        };

        RenderWindow window{
                {windowWidth, windowHeight}, "Arkanoid - Components"};
        FrameTime lastFt{0.f}, currentSlice{0.f};
        bool running{false};
        Manager manager;

        void createBall()
        {
            auto entity = std::make_unique<Entity>(manager);

            entity->addComponent<CPosition>(
                    Vector2f{windowWidth / 2.f, windowHeight / 2.f});
            entity->addComponent<CPhysics>(Vector2f{ballRadius, ballRadius});
            entity->addComponent<CCircle>(ballRadius);

            auto& cPhysics(entity->getComponent<CPhysics>());
            cPhysics.velocity = Vector2f{-ballVelocity, -ballVelocity};
            cPhysics.onOutOfBounds = [&cPhysics](const Vector2f& mSide)
            {
                if(mSide.x != 0.f)
                    cPhysics.velocity.x =
                            std::abs(cPhysics.velocity.x) * mSide.x;

                if(mSide.y != 0.f)
                    cPhysics.velocity.y =
                            std::abs(cPhysics.velocity.y) * mSide.y;
            };

            entity->addGroup(ArkanoidGroup::GBall);

            manager.addEntity(std::move(entity));
        }

        void createBrick(const Vector2f& mPosition)
        {
            Vector2f halfSize{blockWidth / 2.f, blockHeight / 2.f};
            auto entity = std::make_unique<Entity>(manager);

            entity->addComponent<CPosition>(mPosition);
            entity->addComponent<CPhysics>(halfSize);
            entity->addComponent<CRectangle>(halfSize);

            entity->addGroup(ArkanoidGroup::GBrick);

            manager.addEntity(std::move(entity));
        }

        void createPaddle()
        {
            Vector2f halfSize{paddleWidth / 2.f, paddleHeight / 2.f};
            auto entity = std::make_unique<Entity>(manager);

            entity->addComponent<CPosition>(
                    Vector2f{windowWidth / 2.f, windowHeight - 60.f});
            entity->addComponent<CPhysics>(halfSize);
            entity->addComponent<CRectangle>(halfSize);
            entity->addComponent<CPaddleControl>();

            entity->addGroup(ArkanoidGroup::GPaddle);

            manager.addEntity(std::move(entity));
        }

        Game()
        {
            window.setFramerateLimit(240);

            createPaddle();
            createBall();

            for(int iX{0}; iX < countBlocksX; ++iX)
                for(int iY{0}; iY < countBlocksY; ++iY)
                    createBrick(Vector2f{(iX + 1) * (blockWidth + 3) + 22,
                                         (iY + 2) * (blockHeight + 3)});
        }

        void run()
        {
            running = true;

            while(running)
            {
                auto timePoint1(chrono::high_resolution_clock::now());

                window.clear(Color::Black);

                inputPhase();
                updatePhase();
                drawPhase();

                auto timePoint2(chrono::high_resolution_clock::now());
                auto elapsedTime(timePoint2 - timePoint1);
                FrameTime ft{
                        chrono::duration_cast<chrono::duration<float, milli>>(
                                elapsedTime)
                                .count()};

                lastFt = ft;

                auto ftSeconds(ft / 1000.f);
                auto fps(1.f / ftSeconds);

                window.setTitle(
                        "FT: " + to_string(ft) + "\tFPS: " + to_string(fps));
            }
        }

        void inputPhase()
        {
            Event event;
            while(window.pollEvent(event))
            {
                if(event.type == Event::Closed)
                {
                    window.close();
                    break;
                }
            }

            if(Keyboard::isKeyPressed(Keyboard::Key::Escape)) running = false;
        }
        void updatePhase()
        {
            currentSlice += lastFt;
            for(; currentSlice >= ftSlice; currentSlice -= ftSlice)
            {
                manager.refresh();
                manager.update(ftStep);

                auto& paddles(manager.getEntitiesByGroup(GPaddle));
                auto& bricks(manager.getEntitiesByGroup(GBrick));
                auto& balls(manager.getEntitiesByGroup(GBall));

                for(auto& b : balls)
                {
                    for(auto& p : paddles) testCollisionPB(*p, *b);

                    for(auto& br : bricks) testCollisionBB(*br, *b);
                }
            }
        }
        void drawPhase()
        {
            manager.draw(this);
            window.display();
        }
        void render(const Drawable& mDrawable) { window.draw(mDrawable); }
    };

    void CCircle::draw(Game* game) { game->render(shape); }
    void CRectangle::draw(Game* game) { game->render(shape); }
}

int main()
{
    CompositionArkanoid::Game{}.run();
    return 0;
}