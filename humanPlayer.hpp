#pragma once
#include "Player.hpp"

struct HumanPlayer: Player{
    HumanPlayer(short inPlayer = 1): Player(inPlayer, 0) {}
    std::vector<std::vector<uint16_t>> move(const CheckersBoard& board) override {
        return std::vector<std::vector<uint16_t>>{};
    }
};