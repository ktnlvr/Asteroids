#include <iostream>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

using olc::vf2d;

struct Transform {
    olc::vf2d position;
};

struct Ship {
    Transform transform;
};

struct Asteroids : public olc::PixelGameEngine {
    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};

static Asteroids* asteroids;

namespace Procedures {
    void DrawShip();
}


bool Asteroids::OnUserCreate() {
    asteroids = this;
    return OK;
}

bool Asteroids::OnUserUpdate(float deltaTime) {
    Procedures::DrawShip();
    return OK;
}

void Procedures::DrawShip() {
    olc::vf2d a, b, c;
    olc::vf2d center = { (float)asteroids->ScreenWidth() / 2, (float)asteroids->ScreenHeight() / 2};

    a = { center.x, center.y - (float)5 };
    b = { center.x - (float)4, center.y + (float)2.5 };
    c = { center.x + (float)4, center.y + (float)2.5 };

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
