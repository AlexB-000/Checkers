#pragma once
#include "bot.hpp"

struct randomPlayer: Bot{
    randomPlayer(short inPlayer = 1): Bot(inPlayer){}

    std::shared_ptr<Player> copy(short inPlayer = 0) override { return std::make_shared<randomPlayer>(*this); }

    mv move(const CheckersBoard& board, bool print) override {
        std::vector<mv> valid_moves = board.get_valid_moves(board.turn);
        if(!valid_moves.empty()){
            //random numbers generator
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint16_t> dis(0, valid_moves.size() - 1);

            int move_index = dis(gen);
            return valid_moves[move_index];
        }
        return {}; // No valid moves
    }
};