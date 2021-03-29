#include <iostream>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OK 1

struct Asteroids : public olc::PixelGameEngine {
    bool OnUserCreate() override;
    bool OnUserUpdate(float) override;
};


bool Asteroids::OnUserCreate() {
    return OK;
}

bool Asteroids::OnUserUpdate(float deltaTime) {
    return OK;
}


int main(int argc, char* argv[]) {
    Asteroids asteroids = Asteroids();

    if (asteroids.Construct(128, 128, 4, 4))
        asteroids.Start();

    return 0;
}
