#include "netUtils.hpp"

Network buildNetwork(std::string file){
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

Array<float> toArray(const CheckersBoard& state, short turn){
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