#include <chrono>
#include <random>
#include <thread>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "players/checkerZero.hpp"

namespace train_conf{
    uint32_t iterations = 100;
    uint32_t games = 10;
    uint32_t replay_batch_size = 32000;
    uint32_t GDepochs = 4;
    uint32_t GDbatch_size = 128;
    float lr = 0.01f;

    uint32_t minBufferSize = 100000;
    uint32_t maxBufferSize = 150000;
}

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

void simulate(std::vector<Array<float>>* data, std::vector<Array<float>>* target,
    const CheckerZero& player){

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};
    
    CheckerZero player1{1, player};
    CheckerZero player2{-1, player};

    mv move;
    Array<float> state_action;

    float reward;

    // run the game
    uint32_t count = 0;
    bool running = true;
    while (running) {
        count++;

        // get the move and the assiociated reward that shoud be learned
        if (game.turn == 1){
            move = player1.move(game, reward);
        } else {
            move = player2.move(game, reward);
        }

        if (move.size() == 0 || game.checkWin() != 0 || count >= conf::moves_to_draw){
            running = false;
        }
        // The target V value to learn is the result of running the algorithm and exploring the game tree.
        // By using the game tree, the algorithm improves the model prediction.
        // we use (-) the reward because the reward is from the perspective of the player that played the move,
        // and we want to learn with the input being from the perspective of the player that will play NEXT.
        Array<float> v_target {{1}, -reward };

        game.play(move);
        state_action = toArray(game);

        target->push_back(v_target);
        data->push_back(state_action);
    }
}

void collect_data(std::vector<Array<float>>& data, std::vector<Array<float>>& target,
    const CheckerZero& player, uint32_t games=100){

    std::vector<std::vector<Array<float>>> data_threads (games);
    std::vector<std::vector<Array<float>>> target_threads (games);

    std::vector<std::thread> threads;
    threads.reserve(games);

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

CheckerZero train(const CheckerZero& starting_player, uint32_t iteratons=100, uint32_t games=100,
    uint32_t replay_batch_size=10000, uint32_t GDepochs=10, uint32_t GDbatch_size=128, float lr=0.01f){
    
    CheckerZero player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    GD optimizer(&player.model, &loss);

    std::vector<Array<float>> data, target;

    for(uint32_t i = 0; i < iteratons; i++){
        std::cout << "Iteration " << i+1 << "/" << iteratons << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        collect_data(data, target, player, games);

        std::cout << "\tData collected : " << data.size() << " samples" << std::endl;

        float average_game_length = data.size() / static_cast<float>(games * (i + 1));
        std::cout << "\tAverage game length : " << average_game_length << " moves" << std::endl;

        std::vector<Array<float>> batch_data, batch_target;
        fill_batch(data, target, batch_data, batch_target, replay_batch_size);
        optimizer.train(batch_data, batch_target, GDepochs, GDbatch_size, lr, 0, true, true);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::seconds iteration_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "\tIteration time : " << iteration_time.count() << " s" << std::endl;

        if (data.size() > train_conf::maxBufferSize){
            data.erase(data.begin(), data.begin() + (data.size() - train_conf::minBufferSize));
            target.erase(target.begin(), target.begin() + (target.size() - train_conf::minBufferSize));
        }
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
    
    CheckerZero player{1, load_file, 1000, 5.0f, true};

    auto start = std::chrono::high_resolution_clock::now();
    player = train(player, train_conf::iterations, train_conf::games,
                    train_conf::replay_batch_size, train_conf::GDepochs, train_conf::GDbatch_size, train_conf::lr);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::seconds training_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    std::cout << "-- Training finished ---\n";
    std::cout << train_conf::iterations << " iterations : " << train_conf::games << " games / iteration\n";
    std::cout << "Total games played: " << train_conf::iterations * train_conf::games << std::endl;
    std::cout << "Training time: " << training_time.count() << " s" << std::endl;
    // auto params = player.model.get_parameters();
    // for (auto p: params){
    //     p->show();
    // }
    player.model.save(save_file);
}