#include <random>
#include <thread>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "neuralNet.hpp"

void simulate(std::vector<Array<float>>* data, std::vector<Array<float>>* target,
    const NeuralNetPlayer& player, float gamma=0.9f, float exploration=0.1f){

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    std::vector<Array<float>> data1, data2, target1, target2;
    
    NeuralNetPlayer player1{1, player};
    NeuralNetPlayer player2{-1, player};

    std::vector<std::vector<uint16_t>> move;
    Array<float> state_action;
    float v1, v2, v1_next, v2_next;
    float reward = 0.0f;

    for (uint32_t i=0; i < 2; i++){
        if (game.turn == 1){
            move = player1.getAction(game, v1, state_action, exploration);
            data1.push_back(state_action);
        } else {
            move = player2.getAction(game, v2, state_action, exploration);
            data2.push_back(state_action);
        }
        game.play(move);
    }

    bool reached_final_s = false;
    uint32_t count = 0;

    bool running = true;
    while (running) {
        count++;

        if (game.turn == 1){
            move = player1.getAction(game, v1_next, state_action, exploration);

            if (move.size() == 0 || game.checkWin() != 0 || count >= 1000 || reached_final_s){
                if (reached_final_s){
                    running = false;
                }
                reached_final_s = true;
            }
            // set the reward
            if (game.checkWin() == game.turn){
                reward = 1;
            } else if (game.checkWin() == -game.turn){
                reward = -1;
            } else {
                reward = 0;
            }
            if (reached_final_s) {
                v1_next = 0.0f;
            }
            // we compute the target V value to learn
            Array<float> v_target {{1},  reward + gamma * (v1_next) };

            target1.push_back(v_target);
            data1.push_back(state_action);
            v1 = v1_next;
            if (reached_final_s){
                target1.push_back(Array<float>({1}, reward));
            }
        } else {
            move = player2.getAction(game, v2_next, state_action, exploration);

            if (move.size() == 0 || game.checkWin() != 0 || count >= 1000 || reached_final_s){
                if (reached_final_s){
                    running = false;
                }
                reached_final_s = true;
            }
            // set the reward
            if (game.checkWin() == game.turn){
                reward = 1;
            } else if (game.checkWin() == -game.turn){
                reward = -1;
            } else {
                reward = 0;
            }
            if (reached_final_s) {
                v2_next = 0.0f;
            }

            // we compute the target V value to learn
            Array<float> v_target {{1},  reward + gamma * (v2_next) };

            target1.push_back(v_target);
            data1.push_back(state_action);
            v2 = v2_next;
            if (reached_final_s){
                target2.push_back(Array<float>({1}, reward));
            }
        }
        game.play(move);
    }
    data1.insert(data1.end(), data2.begin(), data2.end());
    target1.insert(target1.end(), target2.begin(), target2.end());

    data->insert(data->end(), data1.begin(), data1.end());
    target->insert(target->end(), target1.begin(), target1.end());
}

void collect_data(std::vector<Array<float>>& data, std::vector<Array<float>>& target,
    const NeuralNetPlayer& player, uint32_t games=100, float gamma=0.9f, float exploration=0.1f){

    std::vector<std::vector<Array<float>>> data_threads (games);
    std::vector<std::vector<Array<float>>> target_threads (games);

    std::vector<std::thread> threads;

    for(uint32_t i = 0; i < games; i++){
        threads.emplace_back(simulate, &data_threads[i], &target_threads[i], player, gamma, exploration);
    }
    for (std::thread& t : threads){
        t.join();
    }
    for (uint32_t i = 0; i < games; i++){
        data.insert(data.end(), data_threads[i].begin(), data_threads[i].end());
        target.insert(target.end(), target_threads[i].begin(), target_threads[i].end());
    }
}

NeuralNetPlayer train(const NeuralNetPlayer& starting_player, uint32_t iteratons=100, uint32_t games=100,
    float lr=0.01f, float gamma=0.9f, float exploration=0.1f){
    
    NeuralNetPlayer player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    GD optimizer(&player.model, &loss);

    for(uint32_t i = 0; i < iteratons; i++){
        std::cout << "                      iteration " << i+1 << "/" << iteratons << std::endl;

        std::vector<Array<float>> data;
        std::vector<Array<float>> target;
        collect_data(data, target, player, games, gamma, exploration);

        optimizer.train(data, target, 1, 100, lr, 0);
    }
    return player;
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
    player = train(player, 50, 100, 0.01, 0.9f, 0.1f);

    auto params = player.model.get_parameters();
    for (auto p: params){
        p->show();
    }
    player.model.save(save_file);
}