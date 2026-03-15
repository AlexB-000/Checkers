#include <random>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "neuralNet.hpp"

NeuralNetPlayer train_Q_learning(const NeuralNetPlayer& starting_player, uint32_t games=100,
    float alpha=0.5f, float lr=0.01f, float gamma=0.5f){
    
    NeuralNetPlayer learning_player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    GD optimizer(&learning_player.model, &loss);

    for(uint32_t i = 0; i < games; i++){
        std::cout << "                      playing game " << i << "/" << games << std::endl;

        NeuralNetPlayer adversary{-1, learning_player};

        std::vector<std::vector<uint16_t>> move;

        if (game.turn == -1){
            move = adversary.move(game, false);
            game.play(move);
        }

        Array<float> state_action;
        float q;

        float q_next_move;
        Array<float> next_state_action;
        
        move = learning_player.getAction(game, q, state_action, 1.0f);
        game.play(move);

        // the adversary makes his move
        move = adversary.move(game, false);
        game.play(move);

        bool reached_final_s = false;
        uint32_t count = 0;
        while (!reached_final_s) {
            count++;
            // we get the q value of the next move
            move = learning_player.getAction(game, q_next_move, next_state_action, 1.0f);
            if (move.size() == 0 || game.checkWin() != 0 || count >= 1000){
                reached_final_s = true;
            }
            
            // set the reward
            float reward = 0.0f;
            if (game.checkWin() == game.turn) reward = 1;
            else if (game.checkWin() == -game.turn) reward = -1;

            // we compute the target Q value to learn
            Array<float> q_target {{1},  q + alpha * (reward + gamma * q_next_move - q) };
            
            // learning new q value through stochastic gradient descent
            optimizer.train({state_action}, {q_target}, 1, 1, lr, 0);
            
            game.play(move);
            // the adversary makes his move
            move = adversary.move(game, false);
            game.play(move);

            q = q_next_move;
            state_action = next_state_action;
        }
        
        game.reset();
    }
    return learning_player;
}

int main(int argc, char* argv[]){
    std::string load_file = "";
    std::string save_file = "./NeuralNet_Checkers";
    if (argc > 1){
        if (std::string(argv[1]) == "--load"){
            load_file = std::string(argv[2]);
        } else if (std::string(argv[1]) == "--save"){
            save_file = std::string(argv[2]);
        }
    }
    if (argc > 3){
        if (std::string(argv[3]) == "--load"){
            load_file = std::string(argv[4]);
        } else if (std::string(argv[3]) == "--save"){
            save_file = std::string(argv[4]);
        }
    }
    
    NeuralNetPlayer player{1, load_file};
    player = train_Q_learning(player, 1000, 1.0, 0.001, 0.5f);

    auto params = player.model.get_parameters();
    for (auto p: params){
        p->show();
    }
    player.model.save(save_file);
}