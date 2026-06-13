#pragma once
#include "player.hpp"

struct HumanPlayer: Player{
    HumanPlayer(short inPlayer = 1): Player(inPlayer, 0) {}
    mv move(const CheckersBoard& board) override {
        return mv{};
    }
};