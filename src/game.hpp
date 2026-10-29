#ifndef FLAPPYBIRD_GAME_HPP
#define FLAPPYBIRD_GAME_HPP

#include "config.hpp"
#include "daynight.hpp"

namespace game {

enum State { START, PLAYING, DYING, GAMEOVER, PAUSED };

struct Bird {
    Vector2 pos = {cfg::phys::BIRD_X, cfg::WORLD_H / 2.0f};
    Vector2 vel = {0, 0};
    float rotation = 0.0f; // degrees
    float flapAnim = 0.0f; // 0..1 wing cycle
    bool grounded = false;
};

struct Pipe {
    float x = 0.0f;
    float topH = 0.0f;
    float bottomY = 0.0f;
    bool passed = false;
    bool spawned = false;
};

struct Game {
    State state = START;
    int score = 0;
    int highScore = 0;
    bool isNewBest = false;

    Bird bird;
    Pipe pipes[6];
    float groundScroll = 0.0f;
    float runTime = 0.0f;
    float deathTimer = 0.0f;

    daynight::State daynightSt;

    // Input edges, written once per frame and consumed once per fixed tick.
    bool flapPressed = false;
    bool pausePressed = false;
    bool restartPressed = false;
    bool menuPressed = false;

    bool debug = false;

    void Reset();
    void Die();
    void SpawnPipe();
    void UpdatePipes(float dt);
    void UpdateBird(float dt);
    void CheckCollisions();
    void Flap();
};

extern Game gGame;

void GameInit();
void GameFixedUpdate();                 // fixed 120 Hz simulation
void GameFrameInput(Vector2 worldMouse); // once per rendered frame
void GameRender();

} // namespace game

#endif // FLAPPYBIRD_GAME_HPP
