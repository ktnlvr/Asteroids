#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

// Radius of the biggest asteroid
// BIG_RADIUS = BIG_ASTEROID_RADIUS
// AVERAGE_RADIUS = BIG_RADIUS / 2
// SMOLL_RADIUS = AVERAGE_RADIUS / 4 ?
#define BIG_ROCK_RADIUS 16
// Amount of steps taken to draw the big asteroid 
#define BIG_ROCK_STEPS 16
// Amount of big asteroids
#define BIG_ROCKS_N 16
// Maximum amount of projectiles to be alive
#define PROJECTILE_POOL_SIZE 32
// Radius of one projectile
#define PROJECTILE_RADIUS 2


struct Palette {
    olc::Pixel background, ship, asteroid, projectile;
};

Palette palettes[1] = {
    { olc::BLACK, olc::WHITE, olc::WHITE, olc::WHITE } // contrast
};

Palette* palette;


struct Transform {
    olc::vf2d position;
    float radius;
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
        float projectileSpeed;
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

struct Projectile {
    Transform transform;
    olc::vf2d velocity;
};

inline const Rock Rock::null = Rock({ { {0, 0}, 0, 0 }, { 0, 0 } });

struct Asteroids : public olc::PixelGameEngine {
    Rock rocks[BIG_ROCKS_N];

    Projectile projectiles[PROJECTILE_POOL_SIZE];
    size_t projectileStackCounter = 0;

    float deltaTime;
    Ship ship;
    
    Asteroids() = default;
    olc::vf2d ScreenCenter();
    void RotateVector(olc::vf2d& target, olc::vf2d around, float angle);
    void SummonProjectile(olc::vf2d position, olc::vf2d velocity);
    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};

static Asteroids* asteroids;

namespace Procedures {
    void ProcessInputs();
    void ProcessRocks();
    void ProcessProjectiles();
    void ProcessCollisions();
    void DrawShip();
    void DrawAsteroids();
    void DrawProjectiles();
}


bool Transform::operator&&(Transform& other) {
    // sqrt is usually expensive so just using squares why not
    float distance2 = (other.position - position).mag2();
    return distance2 < (radius + other.radius)* (radius + other.radius);
}

bool Asteroids::OnUserCreate() {
    asteroids = this;
    palette = &palettes[0];

    Ship& ship = asteroids->ship;

    asteroids->rocks[0] = Rock({ ScreenCenter(), BIG_ROCK_RADIUS }, { 0, 20 }, Rock::Size::BIG);
    ship.transform.position = asteroids->ScreenCenter();
    ship.dimensions = { 7, 10 };

    // Choose the highest of two for the collision radius
    ship.transform.radius = ship.dimensions.x < ship.dimensions.y ? ship.dimensions.x : ship.dimensions.y;

    ship.stats.rotationSpeed = 5;
    ship.stats.movementSpeed = 300;
    ship.stats.projectileSpeed = 500;

    asteroids->sAppName = "Asteroids (by Kittenlover229)";

    return OK;
}

inline olc::vf2d Asteroids::ScreenCenter() {
    return { (float)ScreenWidth() / 2, (float)ScreenHeight() / 2 };
}

void Asteroids::SummonProjectile(olc::vf2d position, olc::vf2d velocity) {
    Projectile* bullet = &projectiles[(++projectileStackCounter) % PROJECTILE_POOL_SIZE];

    bullet->transform.position = position;
    bullet->transform.radius = PROJECTILE_RADIUS;
    bullet->velocity = velocity;
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

    Clear(palette->background);
    Procedures::ProcessInputs();
    Procedures::ProcessProjectiles();
    Procedures::ProcessRocks();
    Procedures::ProcessCollisions();
    Procedures::DrawShip();
    Procedures::DrawAsteroids();
    Procedures::DrawProjectiles();

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
    Ship& ship = asteroids->ship;

    /// Rotation is done with A and D keys, can be 0, -1 and 1
    ship.transform.rotation += (asteroids->GetKey(olc::Key::D).bHeld - asteroids->GetKey(olc::Key::A).bHeld) * asteroids->deltaTime * ship.stats.rotationSpeed;

    // Forward direction is up vector (cuz the ship is facing up by default) rotated by ship's rotation
    olc::vf2d forward = { 0, 1 };
    asteroids->RotateVector(forward, olc::vf2d(0, 0), ship.transform.rotation);

    // Velocity is controlled with S and W, can also be 0, -1 and 1
    ship.velocity += forward * ship.stats.movementSpeed * (asteroids->GetKey(olc::Key::S).bHeld - asteroids->GetKey(olc::Key::W).bHeld) * asteroids->deltaTime;
    
    if (asteroids->GetKey(olc::Key::SPACE).bPressed) {
        asteroids->SummonProjectile(ship.transform.position, -forward * ship.stats.projectileSpeed);
    }

    // Since drag hasn't made it yet, just use this
    ship.transform.position += ship.velocity;
    ship.velocity = { 0, 0 };

    // Wrap the ship, doesn't apply to anything else
    WrapPosition(ship.transform.position);
}

void Procedures::ProcessRocks() {
    for (int i = 0; i < BIG_ROCKS_N && (bool)(asteroids->rocks[i].size); ++i) {
        Rock& rock = asteroids->rocks[i];
        rock.transform.position += rock.velocity * asteroids->deltaTime;
        WrapPosition(rock.transform.position);
        rock.transform.rotation += rock.velocity.mag() / rock.transform.radius * asteroids->deltaTime;
    }
}

void Procedures::ProcessProjectiles() {
    for (int i = 0; i < PROJECTILE_POOL_SIZE; ++i) {
        Projectile& self = asteroids->projectiles[i];
        self.transform.position += self.velocity * asteroids->deltaTime;
    }
}

void Procedures::ProcessCollisions() {
    // Doesn't handle ship collisions yet
    for (int i = 0; i < BIG_ROCKS_N; i++) {
        Rock& rock = asteroids->rocks[i];
        for (int j = 0; j < PROJECTILE_POOL_SIZE; j++) {
            Projectile& projectile = asteroids->projectiles[j];

            if (projectile.transform && rock.transform && (bool)(rock.size)) {
                //                   ^ custom operator  ^ ordinary boolean AND
                
                // Kill both rock and projectile
                rock.size = Rock::Size::NONE;
                projectile.transform.radius = 0;
            }
        }
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

    Ship& ship = asteroids->ship;
    olc::vf2d center = ship.transform.position;
    Transform& transform = ship.transform;
    olc::vf2d& dimension = ship.dimensions;

    // It is a bit unordinary, but it has it's own proportions
    a = { center.x, center.y - (float) ship.dimensions.y / 2};
    d = { center.x, center.y + (float) ship.dimensions.y / 3 };
    b = { center.x - (float)ship.dimensions.x / 2, center.y + (float)ship.dimensions.y / 2 };
    c = { center.x + (float)ship.dimensions.x / 2, center.y + (float)ship.dimensions.y / 2 };

    // Rotate all the triangle points
    olc::vf2d* points[4] = { &a, &b, &c, &d };
    for (int i = 0; i < 4; i++)
        asteroids->RotateVector(*points[i], center, transform.rotation);

    asteroids->RotateVector(direction, olc::vf2d{ 0, 0 }, transform.rotation /* rad */);

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
        asteroids->DrawLine(center, center - direction * 16, olc::MAGENTA);

        bool isColliding = false;

        for (int ii = 0; ii < BIG_ROCKS_N; ++ii) 
            isColliding |= asteroids->rocks[ii].transform && ship.transform && (bool)(asteroids->rocks[ii].size);
            //                                             ^ custom operator ^ ordinary boolean AND

        asteroids->DrawCircle(center, transform.radius, isColliding ? olc::RED : olc::GREEN);
#endif

    // Might as well unroll this
    #pragma unroll (9)
    for (int i = 0; i < 9; i++) {
        olc::vi2d offset = offsets[i];

        asteroids->DrawLine(a + offset, b + offset, palette->ship);
        asteroids->DrawLine(b + offset, d + offset, palette->ship);
        asteroids->DrawLine(d + offset, c + offset, palette->ship);
        asteroids->DrawLine(c + offset, a + offset, palette->ship);
    }

}

void Procedures::DrawAsteroids() {
    // Drawing is done in steps and every iteration we rotate the current vector by this 
    // to achieve rotation
    Rock* rocks = asteroids->rocks;
    
    for (int i = 0; i < BIG_ROCKS_N && (bool)(asteroids->rocks[i].size); ++i) {
        float step = 6.28319 / BIG_ROCK_STEPS /* rad */;
        // The loop starts with previous so it is our initial position
        olc::vf2d start = { 0, rocks[i].transform.radius };
        olc::vf2d previous = start;
        asteroids->RotateVector(previous, { 0, 0 }, rocks[i].transform.rotation);
        olc::vf2d current;

        // Move thru all the vertices one by one connecting them
        for (int ii = 1; ii <= BIG_ROCK_STEPS; ii++) {
            current = start;
            asteroids->RotateVector(current, { 0, 0 }, step * ii + rocks[i].transform.rotation);
            asteroids->DrawLine(
                current + rocks[i].transform.position,
                previous + rocks[i].transform.position,
                palette->asteroid
            );
            previous = current;
        }
    }
}

void Procedures::DrawProjectiles() {
    for (int i = 0; i < PROJECTILE_POOL_SIZE; ++i) {
        Projectile& self = asteroids->projectiles[i];
        asteroids->DrawCircle(self.transform.position, self.transform.radius, palette->projectile);
    }
}



int main(int argc, char* argv[]) {
    std::thread thread = std::thread([]() {
        Asteroids asteroids = Asteroids();
        if (asteroids.Construct(256, 256, 2, 2))
            asteroids.Start();
        });

    thread.join();

    return 0;
}
