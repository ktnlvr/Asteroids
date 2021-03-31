#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

// Radius of the biggest asteroid
// BIG_RADIUS = BIG_ASTEROID_RADIUS
// AVERAGE_RADIUS = BIG_RADIUS / 2
// SMOLL_RADIUS = AVERAGE_RADIUS / 4 ?
#define BIG_ROCK_RADIUS 16
// Amount of steps taken to draw the big asteroid 
#define BIG_ROCK_STEPS 12
// Amount of big asteroids
#define BIG_ROCKS_N 16

struct Transform {
    olc::vf2d position;
    float radius;
    // Rotation in radians!
    float rotation;

    // Is colliding with other
    bool operator&&(Transform&);
};

struct Ship {
    Transform transform;
    olc::vf2d dimensions;
    olc::vf2d velocity;

    struct Stats {
        float rotationSpeed;
        float movementSpeed;
    } stats;
};

struct Rock {
    Transform transform;
    olc::vf2d velocity;

    static const Rock null;

    // *softly* The chonk chart
    enum struct Size : char {
        NONE = 0, // indicated that the asteroid is none existent
        BIG = 1,
        AVERAGE = 2,
        SMALL = 3
    } size;

    Rock() : transform({ {0, 0}, 0, 0 }), velocity({ 0, 0 }), size(Rock::Size::NONE) {};
    Rock(Transform transform, olc::vf2d velocity, Rock::Size size = Rock::Size::NONE) : transform(transform), velocity(velocity), size(size) {}

};

inline const Rock Rock::null = Rock({ { {0, 0}, 0, 0 }, { 0, 0 } });

struct Asteroids : public olc::PixelGameEngine {
    Rock rocks[BIG_ROCKS_N];
    float deltaTime;
    Ship ship;
    
    Asteroids() = default;
    olc::vf2d ScreenCenter();
    void RotateVector(olc::vf2d& target, olc::vf2d around, float angle);
    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};

static Asteroids* asteroids;

namespace Procedures {
    void ProcessInputs();
    void ProcessRocks();
    void DrawShip();
    void DrawAsteroids();
}


bool Transform::operator&&(Transform& other) {
    // sqrt is usually expensive so just using squares why not
    float distance2 = (other.position - position).mag2();
    return distance2 < (radius + other.radius)* (radius + other.radius);
}

bool Asteroids::OnUserCreate() {
    asteroids = this;

    asteroids->rocks[0] = Rock({ ScreenCenter(), BIG_ROCK_RADIUS }, { 0, 20 }, Rock::Size::BIG);
    asteroids->ship.transform.position = asteroids->ScreenCenter();
    asteroids->ship.dimensions = { 7, 10 };
    asteroids->ship.transform.radius = asteroids->ship.dimensions.x < asteroids->ship.dimensions.y ? asteroids->ship.dimensions.x : asteroids->ship.dimensions.y;

    asteroids->ship.stats.rotationSpeed = 5;
    asteroids->ship.stats.movementSpeed = 300;

    asteroids->sAppName = "Asteroids (by Kittenlover229)";

    return OK;
}

inline olc::vf2d Asteroids::ScreenCenter() {
    return { (float)ScreenWidth() / 2, (float)ScreenHeight() / 2 };
}

inline void WrapPosition(olc::vf2d& v) {
    if (v.y > asteroids->ScreenHeight())
        v.y -= asteroids->ScreenHeight();
    else if (v.y < 0)
        v.y += asteroids->ScreenHeight();

    if (v.x > asteroids->ScreenWidth())
        v.x -= asteroids->ScreenWidth();
    else if (v.x < 0)
        v.x += asteroids->ScreenWidth();
}

bool Asteroids::OnUserUpdate(float deltaTime) {
    this->deltaTime = deltaTime;

    Clear(olc::BLACK);
    Procedures::ProcessInputs();
    Procedures::ProcessRocks();
    Procedures::DrawShip();
    Procedures::DrawAsteroids();

    return OK;
}

void Asteroids::RotateVector(olc::vf2d& target, olc::vf2d around, float angle) {
    float cosangle = cos(angle);
    float sinangle = sin(angle);

    // moving point to the origin
    target -= around;

    olc::vf2d rotated = { target.x * cosangle - target.y * sinangle, target.x * sinangle + target.y * cosangle };

    // shift back to origin
    rotated += around;

    target = rotated;
}

void Procedures::ProcessInputs() {
    /// Rotation is done with A and D keys, can be 0, -1 and 1
    asteroids->ship.transform.rotation += (asteroids->GetKey(olc::Key::D).bHeld - asteroids->GetKey(olc::Key::A).bHeld) * asteroids->deltaTime * asteroids->ship.stats.rotationSpeed;

    // Forward direction is up vector (cuz the ship is facing up by default) rotated by ship's rotation
    olc::vf2d forward = { 0, 1 };
    asteroids->RotateVector(forward, olc::vf2d(0, 0), asteroids->ship.transform.rotation);

    // Velocity is controlled with S and W, can also be 0, -1 and 1
    asteroids->ship.velocity += forward * asteroids->ship.stats.movementSpeed * (asteroids->GetKey(olc::Key::S).bHeld - asteroids->GetKey(olc::Key::W).bHeld) * asteroids->deltaTime;
    
    // Since drag hasn't made it yet, just use this
    asteroids->ship.transform.position += asteroids->ship.velocity;
    asteroids->ship.velocity = { 0, 0 };

    // Wrap the ship, doesn't apply to anything else
    WrapPosition(asteroids->ship.transform.position);
}

void Procedures::ProcessRocks() {
    for (int i = 0; i < BIG_ROCKS_N && (bool)(asteroids->rocks[i].size); ++i) {
        asteroids->rocks[i].transform.position += asteroids->rocks[i].velocity * asteroids->deltaTime;
        WrapPosition(asteroids->rocks[i].transform.position);
    }
}

void Procedures::DrawShip() {
    /* Ship vertices layout
    
           ^ direction
           |
           a
          /|\
         / | \
        /  |  \
       /__/-\__\
      //   d   \\
      ^         ^
      b         c

    */

    olc::vf2d a, b, c, d, direction = { 0, 1 };
    olc::vf2d center = asteroids->ship.transform.position;
    Transform* transform = &asteroids->ship.transform;
    olc::vf2d* dimension = &asteroids->ship.dimensions;

    // It is a bit unordinary, but it has it's own proportions
    a = { center.x, center.y - (float) asteroids->ship.dimensions.y / 2};
    d = { center.x, center.y + (float) asteroids->ship.dimensions.y / 3 };
    b = { center.x - (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };
    c = { center.x + (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };

    // Rotate all the triangle points
    asteroids->RotateVector(a, center, transform->rotation);
    asteroids->RotateVector(d, center, transform->rotation);
    asteroids->RotateVector(b, center, transform->rotation);
    asteroids->RotateVector(c, center, transform->rotation);
    asteroids->RotateVector(direction, olc::vf2d{ 0, 0 }, transform->rotation /* rad */);

    // No need for spaghetti ifs, draw all of them
    // Too many draw calls is the exact amount of draw calls needed
    // KISS

    /* The resulting grid of fake ships
        x - real
        o - fake

        o -- o -- o
        :    :    :
        o -- x -- o
        :    :    :
        o -- o -- o
    */

    olc::vi2d offsets[9] = {
        olc::vi2d { 0, 0 },
        olc::vi2d { 0, asteroids->ScreenHeight() },
        olc::vi2d { asteroids->ScreenWidth(), 0 },
        olc::vi2d { 0, -asteroids->ScreenHeight() },
        olc::vi2d { -asteroids->ScreenWidth(), 0 },
        olc::vi2d { asteroids->ScreenWidth(), asteroids->ScreenHeight() },
        olc::vi2d { asteroids->ScreenWidth(), -asteroids->ScreenHeight() },
        olc::vi2d { -asteroids->ScreenWidth(), asteroids->ScreenHeight() },
        olc::vi2d { -asteroids->ScreenWidth(), -asteroids->ScreenHeight() },
    };

#ifndef NDEBUG
        // Debug direction and collision radius
        asteroids->DrawLine(center, center - direction * 16, olc::DARK_GREY);

        bool isColliding = false;

        for (int ii = 0; ii < BIG_ROCKS_N; ++ii) 
            isColliding |= asteroids->rocks[ii].transform && (asteroids->ship.transform);

        asteroids->DrawCircle(center, transform->radius, isColliding ? olc::RED : olc::GREEN);
#endif

    // Might as well unroll this
    #pragma unroll (9)
    for (int i = 0; i < 9; i++) {
        olc::vi2d offset = offsets[i];

        asteroids->DrawLine(a + offset, b + offset);
        asteroids->DrawLine(b + offset, d + offset);
        asteroids->DrawLine(d + offset, c + offset);
        asteroids->DrawLine(c + offset, a + offset);
    }

}

void Procedures::DrawAsteroids() {
    // Drawing is done in steps and every iteration we rotate the current vector by this 
    // to achieve rotation
    float step = 6.28319 / BIG_ROCK_STEPS /* rad */;
    Rock* rocks = asteroids->rocks;
    
    for (int i = 0; i < BIG_ROCKS_N && (bool)(asteroids->rocks[i].size); ++i) {
        // The loop starts with previous so it is our initial position
        olc::vf2d previous = { 0, rocks[i].transform.radius };
        olc::vf2d current;

        // Move thru all the vertices one by one connecting them
        for (int ii = 0; ii < BIG_ROCK_STEPS; ii++) {
            current = previous;
            asteroids->RotateVector(current, { 0, 0 }, step);
            asteroids->DrawLine(
                previous + rocks[i].transform.position,
                current + rocks[i].transform.position, olc::GREY);
            previous = current;
        }
    }
}



int main(int argc, char* argv[]) {
    Asteroids asteroids = Asteroids();

    if (asteroids.Construct(256, 256, 2, 2))
        asteroids.Start();

    return 0;
}
