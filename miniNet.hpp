#pragma once
#include "../../Cpp_AI_lib/Cpp_AI.hpp"
#include "neuralNet.hpp"
#include "minimax.hpp"
#include "conf.hpp"

struct MiniNet: public NeuralNetPlayer, public Minimax{

    MiniNet(short inPlayer, std::string file = "", int inDepth=4, bool inPrunning=true):
    NeuralNetPlayer(inPlayer, file), Minimax(inPlayer, inDepth, inPrunning) {}

    MiniNet(const MiniNet& other): NeuralNetPlayer(other), Minimax(other) {}

    MiniNet(short inPlayer, const MiniNet& other): NeuralNetPlayer(inPlayer, other),
                                                    Minimax(other) {}

    float score_position(const CheckersBoard& board, short turn) override {
        return model.forward(toArray(board, -turn)) [0];
    }

    std::vector<std::vector<uint16_t>> move(const CheckersBoard& board) override {
        float score = 0;
        std::vector<std::vector<uint16_t>> move = minimax(board, player, score, _depth, -INFINITY, INFINITY, true, _prunning);
        std::cout << "move score : " << score << "\n";
        return move;
    }
};