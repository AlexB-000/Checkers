#pragma once
#include "player.hpp"

struct HumanPlayer: Player{
    HumanPlayer(short inPlayer = 1): Player(inPlayer, 0) {}
    HumanPlayer(short inPlayer, HumanPlayer& other): Player(inPlayer, 0) {}

    std::shared_ptr<Player> copy(short inPlayer = 0) override { return std::make_shared<HumanPlayer>(inPlayer, *this); }

    mv move(const CheckersBoard& board, bool print) override {
        return mv{};
    }
};