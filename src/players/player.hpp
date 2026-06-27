#pragma once
#include <memory>
#include "../CheckersBoard.hpp"

struct Player{
    Player(short inPlayer = 1, char inType=0): player(inPlayer), type(inType) {}

    virtual std::shared_ptr<Player> copy(short inPlayer=0) = 0;

    virtual mv move(const CheckersBoard& board, bool print=true) = 0;

    short player = 1;
    char type = 0; // 0 : human, 1 : bot
};