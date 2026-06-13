#include <random>
#include <thread>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "players/neuralNet.hpp"

void fill_batch(const std::vector<Array<float>>& data, const std::vector<Array<float>>& target,
    std::vector<Array<float>>& batch_data, std::vector<Array<float>>& batch_target, uint32_t batch_size){
    
    if (data.size() == 0) return;
    if (data.size() < batch_size) batch_size = data.size();

    batch_data.clear();
    batch_target.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, data.size() - 1);

    for (uint32_t i = 0; i < batch_size; i++){
        uint32_t idx = dis(gen);
        batch_data.push_back(data[idx]);
        batch_target.push_back(target[idx]);
    }
}

inline float get_reward(const CheckersBoard& game, short turn){
    if (game.winner == turn){
        return 1.0f;
    } else if (game.winner == -turn){
        return -1.0f;
    } else {
        return 0.0f;
    }
}

void simulate(std::vector<Array<float>>* data, std::vector<Array<float>>* target,
    const NeuralNetPlayer& player, float gamma=0.9f, float exploration=0.1f){

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    std::vector<Array<float>> local_data, local_target;
    
    NeuralNetPlayer player1{1, player};
    NeuralNetPlayer player2{-1, player};

    mv move;
    Array<float> state_action;
    float v1, v2, v1_next, v2_next;
    float reward = 0.0f;

    for (uint32_t i=0; i < 2; i++){
        if (game.turn == 1){
            move = player1.getAction(game, v1, state_action, exploration);
            local_data.push_back(state_action);
        } else {
            move = player2.getAction(game, v2, state_action, exploration);
            local_data.push_back(state_action);
        }
        game.play(move);
    }

    bool reached_final_s = false;
    uint32_t count = 0;

    // run the game
    bool running = true;
    while (running) {
        count++;

        if (game.turn == 1){
            move = player1.getAction(game, v1_next, state_action, exploration);

            if (move.size() == 0 || game.checkWin() != 0 || count >= conf::moves_to_draw || reached_final_s){
                if (reached_final_s){
                    running = false;
                }
                reached_final_s = true;
            }
            // set the reward
            reward = get_reward(game, game.turn);

            if (reached_final_s) {
                v1_next = 0.0f;
            }
            // we compute the target V value to learn
            Array<float> v_target {{1},  reward + gamma * (v1_next) };

            local_target.push_back(v_target);
            local_data.push_back(state_action);
            v1 = v1_next;
            if (reached_final_s){
                local_target.push_back(Array<float>({1}, reward));
            }
        } else {
            move = player2.getAction(game, v2_next, state_action, exploration);

            if (move.size() == 0 || game.checkWin() != 0 || count >= conf::moves_to_draw || reached_final_s){
                if (reached_final_s){
                    running = false;
                }
                reached_final_s = true;
            }
            // set the reward
            reward = get_reward(game, game.turn);

            if (reached_final_s) {
                v2_next = 0.0f;
            }

            // we compute the target V value to learn
            Array<float> v_target {{1},  reward + gamma * (v2_next) };

            local_target.push_back(v_target);
            local_data.push_back(state_action);
            v2 = v2_next;
            if (reached_final_s){
                local_target.push_back(Array<float>({1}, reward));
            }
        }
        game.play(move);
    }
    data->insert(data->end(), local_data.begin(), local_data.end());
    target->insert(target->end(), local_target.begin(), local_target.end());
}

void collect_data(std::vector<Array<float>>& data, std::vector<Array<float>>& target,
    const NeuralNetPlayer& player, uint32_t games=100, float gamma=0.9f, float exploration=0.1f){

    std::vector<std::vector<Array<float>>> data_threads (games);
    std::vector<std::vector<Array<float>>> target_threads (games);

    std::vector<std::thread> threads;
    threads.reserve(games);

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
    uint32_t replay_batch_size=10000, uint32_t GDepochs=10, uint32_t GDbatch_size=128, float lr=0.01f, float gamma=0.9f, float exploration=0.1f){
    
    NeuralNetPlayer player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    GD optimizer(&player.model, &loss);

    std::vector<Array<float>> data, target;

    for(uint32_t i = 0; i < iteratons; i++){
        std::cout << "Iteration " << i+1 << "/" << iteratons << std::endl;

        collect_data(data, target, player, games, gamma, exploration);

        std::cout << "\tData collected : " << data.size() << " samples" << std::endl;

        float average_game_length = data.size() / static_cast<float>(games * (i + 1));
        std::cout << "\tAverage game length : " << average_game_length << " moves" << std::endl;

        std::vector<Array<float>> batch_data, batch_target;
        fill_batch(data, target, batch_data, batch_target, replay_batch_size);
        optimizer.train(batch_data, batch_target, GDepochs, GDbatch_size, lr);
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
    player = train(player, 100, 100, 10000, 5, 128, 0.01, 0.9f, 0.1f);

    // auto params = player.model.get_parameters();
    // for (auto p: params){
    //     p->show();
    // }
    player.model.save(save_file);
}