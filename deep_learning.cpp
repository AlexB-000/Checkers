#include <random>
#include <thread>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "neuralNet.hpp"

void simulate(std::vector<Array<float>>* data, std::vector<Array<float>>* target,
    const NeuralNetPlayer& player){

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    std::vector<Array<float>> data1;
    std::vector<Array<float>> data2;
    
    NeuralNetPlayer player1{1, player};
    NeuralNetPlayer player2{-1, player};

    std::vector<std::vector<uint16_t>> move;
    Array<float> state_action;
    float q;

    bool reached_final_s = false;
    uint32_t count = 0;
    while (!reached_final_s) {
        count++;

        if (game.turn == 1){
            move = player1.getAction(game, q, state_action, 1.0f);
            data1.push_back(state_action);
        } else {
            move = player2.getAction(game, q, state_action, 1.0f);
            data2.push_back(state_action);
        }
        if (move.size() == 0 || game.checkWin() != 0 || count >= 1000){
            reached_final_s = true;
        }

        game.play(move);
    }

    data->insert(data->end(), data1.begin(), data1.end());
    data->insert(data->end(), data2.begin(), data2.end());

    float reward = game.checkWin();
    std::vector<Array<float>> game_target (data1.size(), Array<float>{{1}, reward});
    game_target.insert(game_target.end(), data2.size(), Array<float>{{1}, -reward});

    target->insert(target->end(), game_target.begin(), game_target.end());
}

void collect_data(std::vector<Array<float>>& data, std::vector<Array<float>>& target,
    const NeuralNetPlayer& player, uint32_t games=100){

    std::vector<std::vector<Array<float>>> data_threads (games);
    std::vector<std::vector<Array<float>>> target_threads (games);

    std::vector<std::thread> threads;

    for(uint32_t i = 0; i < games; i++){
        threads.emplace_back(simulate, &data_threads[i], &target_threads[i], player);
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
    float lr=0.01f){
    
    NeuralNetPlayer player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    GD optimizer(&player.model, &loss);

    for(uint32_t i = 0; i < iteratons; i++){
        std::cout << "                      iteration " << i+1 << "/" << iteratons << std::endl;

        std::vector<Array<float>> data;
        std::vector<Array<float>> target;
        collect_data(data, target, player, games);

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
    player = train(player, 50, 100, 0.01);

    auto params = player.model.get_parameters();
    for (auto p: params){
        p->show();
    }
    player.model.save(save_file);
}