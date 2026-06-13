#pragma once
#include "player.hpp"

struct Bot: public Player{
    Bot(short inPlayer = 1): Player(inPlayer, 1) {}
    mv move(const CheckersBoard& board) override {
        return mv{};
    }
};