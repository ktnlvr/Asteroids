#include <iostream>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

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
};

struct Asteroids : public olc::PixelGameEngine {
    float deltaTime;
    Ship ship;

    void RotateVector(olc::vf2d& target, olc::vf2d around, float angle);
    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};

static Asteroids* asteroids;

namespace Procedures {
    void DrawShip();
}


bool Transform::operator&&(Transform& other) {
    return (other.position - position).mag2() < other.radius * other.radius;
}

bool Asteroids::OnUserCreate() {
    asteroids = this;
    asteroids->ship.transform.position = { (float)asteroids->ScreenWidth() / 2, (float)asteroids->ScreenHeight() / 2 };
    asteroids->ship.dimensions = { 14, 20 };
    asteroids->ship.transform.radius = asteroids->ship.dimensions.x < asteroids->ship.dimensions.y ? asteroids->ship.dimensions.x : asteroids->ship.dimensions.y;
    return OK;
}

bool Asteroids::OnUserUpdate(float deltaTime) {
    this->deltaTime = deltaTime;
    this->ship.transform.rotation += deltaTime;

    Clear(olc::BLACK);
    Procedures::DrawShip();

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

void Procedures::DrawShip() {
    olc::vf2d a, b, c, direction = { 0, 1 };
    olc::vf2d center = asteroids->ship.transform.position;
    Transform* transform = &asteroids->ship.transform;
    olc::vf2d* dimension = &asteroids->ship.dimensions;

    a = { center.x, center.y - (float) asteroids->ship.dimensions.y / 2};
    b = { center.x - (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };
    c = { center.x + (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };

    asteroids->RotateVector(a, center, transform->rotation);
    asteroids->RotateVector(b, center, transform->rotation);
    asteroids->RotateVector(c, center, transform->rotation);
    asteroids->RotateVector(direction, center, transform->rotation + 0.79 /* rad */);

    asteroids->DrawLine(center, direction, olc::RED);
    asteroids->DrawCircle(center, transform->radius, olc::GREEN);
    asteroids->DrawLine(a, b);
    asteroids->DrawLine(b, c);
    asteroids->DrawLine(c, a);
}


int main(int argc, char* argv[]) {
    Asteroids asteroids = Asteroids();

    if (asteroids.Construct(256, 256, 2, 2))
        asteroids.Start();

    return 0;
}
