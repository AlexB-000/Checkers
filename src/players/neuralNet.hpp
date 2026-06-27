#pragma once
#include "CppAI/Cpp_AI.hpp"
#include "bot.hpp"
#include "../conf.hpp"

constexpr uint16_t input_size = conf::board_size[0] * conf::board_size[1] * 2;

Network buildNetwork(std::string file = ""){
    Network model;
    model.stackLayer(std::make_shared<Linear>(input_size, 256));
    model.stackLayer(std::make_shared<ReLU>(256));
    model.stackLayer(std::make_shared<Linear>(256, 128));
    model.stackLayer(std::make_shared<ReLU>(128));
    model.stackLayer(std::make_shared<Linear>(128, 64));
    model.stackLayer(std::make_shared<ReLU>(64));
    model.stackLayer(std::make_shared<Linear>(64, 1));
    model.stackLayer(std::make_shared<Tanh>(1));
    if (file != "") {
        model.load(file);
    }
    return model;
}

Array<float> toArray(const CheckersBoard& state, short turn = 0){
    if (turn == 0) turn = state.turn;

    Array<float> encoded_state {{ input_size }, 0.0f};
    
    const uint32_t queen_offset = state.size[0] * state.size[1] / 2;
    const uint32_t player_offset = state.size[0] * state.size[1];

    const short friend_pawn = turn;
    const short enemy_pawn = -turn;
    const short friend_queen = turn * 2;
    const short enemy_queen = -friend_queen;

    // top player
    if (turn == 1){
        for (uint16_t i=0; i < state.size[0]; i++){
            for (uint16_t j=!(i%2); j < state.size[1]; j+=2){
                const uint32_t idx = (i * state.size[1] + j) / 2;
                
                if (state.board[i][j] == friend_pawn){
                    encoded_state[idx] = 1;

                } else if (state.board[i][j] == friend_queen){
                    encoded_state[idx + queen_offset ] = 1;

                } else if (state.board[i][j] == enemy_pawn){
                    encoded_state[idx + player_offset ] = 1;

                } else if (state.board[i][j] == enemy_queen){
                    encoded_state[idx + queen_offset + player_offset ] = 1;
                }
            }
        }
    }
    //bottom player
    else {
        for (uint16_t i=0; i < state.size[0]; i++){
            for (uint16_t j=!(i%2); j < state.size[1]; j+=2){
                const uint32_t idx = ((state.size[0] - i - 1) * state.size[1] + (state.size[1] - 1 - j)) / 2;

                if (state.board[i][j] == friend_pawn){
                    encoded_state[idx] = 1;

                } else if (state.board[i][j] == friend_queen){
                    encoded_state[idx + queen_offset ] = 1;

                } else if (state.board[i][j] == enemy_pawn){
                    encoded_state[idx + player_offset ] = 1;

                } else if (state.board[i][j] == enemy_queen){
                    encoded_state[idx + queen_offset + player_offset ] = 1;
                }
            }
        }
    }
    
    return encoded_state;
}

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
            state_action = toArray(board);
            q = model.forward(state_action)[0];
            return valid_moves[idx];
        }

        std::vector<float> scores (valid_moves.size(), 0.0f);
        std::vector<Array<float>> states (valid_moves.size());
        
        for (uint32_t i=0; i < valid_moves.size(); i++){
            const mv& move = valid_moves[i];

            CheckersBoard stateCopy = board;
            stateCopy.play(move);
            states[i] = toArray(stateCopy);
            scores[i] = model.forward(states[i]) [0];
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
        Array<float> b;
        mv move = getAction(board, q, b, 0);
        if (print)
        std::cout << "Q value : " << q << std::endl;
        return move;
    }
};
