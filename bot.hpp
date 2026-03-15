#pragma once
#include "Player.hpp"

struct Bot: public Player{
    Bot(short inPlayer = 1): Player(inPlayer, 1) {}
    std::vector<std::vector<uint16_t>> move(const CheckersBoard& board) override {
        return std::vector<std::vector<uint16_t>>{};
    }
};