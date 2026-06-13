#pragma once
#include "../CheckersBoard.hpp"

struct Player{
    Player(short inPlayer = 1, char inType=0): player(inPlayer), type(inType) {}

    virtual mv move(const CheckersBoard& board) = 0;

    short player = 1;
    char type = 0; // 0 : human, 1 : bot
};