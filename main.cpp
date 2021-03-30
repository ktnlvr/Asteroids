#include <iostream>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

struct Transform {
    olc::vf2d position;
};

struct Ship {
    Transform transform;
    olc::vf2d dimensions;
};

struct Asteroids : public olc::PixelGameEngine {
    Ship ship;

    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};

static Asteroids* asteroids;

namespace Procedures {
    void DrawShip();
}


bool Asteroids::OnUserCreate() {
    asteroids = this;
    asteroids->ship.transform.position = { (float)asteroids->ScreenWidth() / 2, (float)asteroids->ScreenHeight() / 2 };
    asteroids->ship.dimensions = { 7, 10 };
    return OK;
}

bool Asteroids::OnUserUpdate(float deltaTime) {
    Procedures::DrawShip();
    return OK;
}

void Procedures::DrawShip() {
    olc::vf2d a, b, c;
    olc::vf2d center = asteroids->ship.transform.position;

    a = { center.x, center.y - (float) asteroids->ship.dimensions.y / 2};
    b = { center.x - (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };
    c = { center.x + (float)asteroids->ship.dimensions.x / 2, center.y + (float)asteroids->ship.dimensions.y / 2 };

    asteroids->DrawLine(a, b);
    asteroids->DrawLine(b, c);
    asteroids->DrawLine(c, a);
}


int main(int argc, char* argv[]) {
    Asteroids asteroids = Asteroids();

    if (asteroids.Construct(128, 128, 4, 4))
        asteroids.Start();

    return 0;
}
