#pragma once
#include "CppAI/Cpp_AI.hpp"
#include "../configuration.hpp"
#include "bot.hpp"
#include "utils/netUtils.hpp"

struct NeuralNetPlayer: virtual public Bot{
    Network model;

    NeuralNetPlayer(short inPlayer, std::string file = ""): Bot(inPlayer), model(buildNetwork(file)) {}

    NeuralNetPlayer(const NeuralNetPlayer& other): Bot(other.player), model(other.model) {}

    NeuralNetPlayer(short inPlayer, const NeuralNetPlayer& other): Bot(inPlayer), model(other.model) {}

    NeuralNetPlayer& operator=(const NeuralNetPlayer& other){
        player = other.player;
        model = other.model;
        return (*this);
    }

    std::shared_ptr<Player> copy(short inPlayer = 0) override { return std::make_shared<NeuralNetPlayer>(inPlayer, *this); }

    mv getAction(const CheckersBoard& board,
        float& q, Array<float>& state_action, float exploration=0.01f, bool show_scores = false) {
        std::vector<mv> valid_moves = board.get_valid_moves(board.turn);

        if (valid_moves.size() == 0){
            // final state
            q = 0.0f;
            return {};
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0, 1);
        if (dis(gen) < exploration) {
            // exploration : we choose a random move
            std::uniform_int_distribution<uint32_t> dis_move(0, valid_moves.size() - 1);
            uint32_t idx = dis_move(gen);
            
            CheckersBoard stateCopy = board;
            stateCopy.play(valid_moves[idx]);
            state_action = toArray(stateCopy);

            q = (*model.forward(state_action).data_ptr)[0];
            return valid_moves[idx];
        }

        std::vector<float> scores (valid_moves.size(), 0.0f);
        std::vector<Array<float>> states;
        states.reserve(valid_moves.size());
        for (uint32_t i=0; i < valid_moves.size(); i++){
            const mv& move = valid_moves[i];

            CheckersBoard stateCopy = board;
            stateCopy.play(move);
            states.emplace_back(toArray(stateCopy));
            scores.emplace_back((*model.forward(states[i]).data_ptr)[0]);
            if (show_scores) std::cout << scores[i] << " ";
        }
        if (show_scores) std::cout << std::endl;
        
        std::vector<uint32_t> options {};
        q = *std::max_element(scores.begin(), scores.end());
        for (uint32_t i=0; i < valid_moves.size(); i++){
            if (scores[i] == q) {
               options.push_back(i);
            }
        }
        std::uniform_int_distribution<uint32_t> dis_move(0, options.size() - 1);
        uint32_t idx = options[dis_move(gen)];
        state_action = states[idx];
        return valid_moves[idx];
    }

    mv move(const CheckersBoard& board, bool print) override {
        float q;
        Array<float> b{{input_size}, 0.0f};
        mv move = getAction(board, q, b, 0);
        if (print)
        std::cout << "Q value : " << q << std::endl;
        return move;
    }
};
