#pragma once
#include "../Cpp_AI_lib/Cpp_AI.hpp"
#include "bot.hpp"
#include "conf.hpp"

Network buildNetwork(std::string file = ""){
    Network model;
    model.stackLayer(std::make_shared<Linear>(conf::board_size[0] * conf::board_size[1] * 2, 64));
    model.stackLayer(std::make_shared<ReLU>(64));
    model.stackLayer(std::make_shared<Linear>(64, 64));
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

    Array<float> encoded_state {{ static_cast<uint>(state.size[0] * state.size[1] * 2) }, 0.0f};
    
    const uint32_t queen_offset = state.size[0] * state.size[1];
    const short friend_pawn = turn;
    const short enemy_pawn = -turn;
    const short friend_queen = turn * 2;
    const short enemy_queen = -friend_queen;

    // top player
    if (turn == 1){
        for (uint16_t i=0; i < state.size[0]; i++){
            for (uint16_t j=0; j < state.size[1]; j++){
                const uint32_t idx = i * state.size[1] + j;
                
                if (state.board[i][j] == friend_pawn){
                    encoded_state[idx] = 1;

                } else if (state.board[i][j] == friend_queen){
                    encoded_state[idx + queen_offset ] = 1;

                } else if (state.board[i][j] == enemy_pawn){
                    encoded_state[idx] = -1;

                } else if (state.board[i][j] == enemy_queen){
                    encoded_state[idx + queen_offset ] = -1;
                }
            }
        }
    }
    //bottom player
    else {
        for (uint16_t i=0; i < state.size[0]; i++){
            for (uint16_t j=0; j < state.size[1]; j++){
                const uint32_t idx = (state.size[0] - i - 1) * state.size[1] + (state.size[1] - 1 - j);

                if (state.board[i][j] == friend_pawn){
                    encoded_state[idx] = 1;

                } else if (state.board[i][j] == friend_queen){
                    encoded_state[idx + queen_offset ] = 1;

                } else if (state.board[i][j] == enemy_pawn){
                    encoded_state[idx] = -1;

                } else if (state.board[i][j] == enemy_queen){
                    encoded_state[idx + queen_offset ] = -1;
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

    std::vector<std::vector<uint16_t>> getAction(const CheckersBoard& board,
        float& q, Array<float>& state_action, float exploration=0.01f, bool show_scores = false) {
        std::vector<std::vector<std::vector<uint16_t>>> valid_moves = board.get_valid_moves(board.turn);

        if (valid_moves.size() == 0){
            // final state
            q = 0.0f;
            return {};
        }

        std::vector<float> scores (valid_moves.size(), 0.0f);
        std::vector<Array<float>> states (valid_moves.size());
        
        for (uint32_t i=0; i < valid_moves.size(); i++){
            const std::vector<std::vector<uint16_t>>& move = valid_moves[i];

            CheckersBoard stateCopy = board;
            stateCopy.play(move);
            states[i] = toArray(stateCopy);
            scores[i] = model.forward(states[i]) [0];
            if (show_scores) std::cout << scores[i] << " ";
        }
        if (show_scores) std::cout << std::endl;
        
        // rescale over [0; +inf[
        float min_score = INFINITY;
        for (uint32_t i=0; i < valid_moves.size(); i++){
            if (scores[i] < min_score){
                min_score = scores[i];
            }
        }
        min_score -= exploration;
        float sum = 0.0f;
        for (uint32_t i=0; i < valid_moves.size(); i++){
            // we put score[i] n [0; +inf[
            scores[i] -= min_score;
            sum += scores[i];
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0, sum);
        float rand = dis(gen);
        
        std::vector<std::vector<uint16_t>> move;
        float cumulated = 0.0f;
        for (uint32_t i=0; i < valid_moves.size(); i++){
            cumulated += scores[i];
            if (rand <= cumulated) {
                move = valid_moves[i];
                // we get the score before the correction
                q = scores[i] + min_score;
                state_action = states[i];
                break;
            }
        }
        return move;
    }

    std::vector<std::vector<uint16_t>> move(const CheckersBoard& board, bool show_q) {
        float q;
        Array<float> b;
        std::vector<std::vector<uint16_t>> move = getAction(board, q, b, 0.01f);
        if (show_q)
        std::cout << "Q value : " << q << std::endl;
        return move;
    }

    std::vector<std::vector<uint16_t>> move(const CheckersBoard& board) override {
        float q;
        Array<float> b;
        std::vector<std::vector<uint16_t>> move = getAction(board, q, b, 0.01f, true);
        std::cout << "Q value : " << q << std::endl;
        return move;
    }
};
