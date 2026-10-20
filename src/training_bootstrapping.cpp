#include <chrono>
#include <random>
#include <thread>
#include <memory>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "players/checkerZero.hpp"

using algo = CheckerZero;

namespace train_conf{
    constexpr uint32_t iterations = 10;
    constexpr uint32_t games = 10;
    constexpr uint32_t replay_batch_size = 32000;
    constexpr uint32_t GDepochs = 4;
    constexpr uint32_t GDbatch_size = 128;
    constexpr float lr = 0.01f;
    constexpr float momentum = 0.9f;

    constexpr uint32_t minBufferSize = 100000;
    constexpr uint32_t maxBufferSize = 150000;
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
        batch_data.emplace_back(data[idx]);
        batch_target.emplace_back(target[idx]);
    }
}

//MARK: save / load replay
void saveReplayBuffer(const std::vector<Array<float>>& data, const std::vector<Array<float>>& target) {
    std::ofstream dataFile("../../bootstrap_save/replay_buffer_data.bin", std::ios::binary);
    std::ofstream targetFile("../../bootstrap_save/replay_buffer_target.bin", std::ios::binary);
	if (!dataFile) {
		std::cerr << "WARNING - Error opening file for saving replay buffer data" << std::endl;
	}
	if (!targetFile) {
		std::cerr << "WARNING - Error opening file for saving replay buffer target" << std::endl;
	}
    if (!dataFile || !targetFile) return ;

    for (const Array<float>& d : data) {
        dataFile.write(reinterpret_cast<const char*>((*d.data_ptr).data()), (*d.data_ptr).size() * sizeof(float));
    }
    for (const Array<float>& t : target) {
        targetFile.write(reinterpret_cast<const char*>((*t.data_ptr).data()), (*t.data_ptr).size() * sizeof(float));
    }
    std::cout << "Successfully saved replay buffer" << std::endl;
}

void loadReplayBuffer(std::vector<Array<float>>& data, std::vector<Array<float>>& target) {
    std::ifstream dataFile("../../bootstrap_save/replay_buffer_data.bin", std::ios::binary);
    std::ifstream targetFile("../../bootstrap_save/replay_buffer_target.bin", std::ios::binary);
	if (!dataFile) {
		std::cerr << "WARNING - Error opening file for loading replay buffer data" << std::endl;
	}
	if (!targetFile) {
		std::cerr << "WARNING - Error opening file for loading replay buffer target" << std::endl;
	}
    if (!dataFile || !targetFile) return ;

    while (dataFile.peek() != EOF) {
        Array<float> d(std::vector<uint32_t>{input_size});
        dataFile.read(reinterpret_cast<char*>((*d.data_ptr).data()), sizeof(float)*input_size);
        data.push_back(d);
    }
    while (targetFile.peek() != EOF) {
        Array<float> t(std::vector<uint32_t>{1});
        targetFile.read(reinterpret_cast<char*>((*t.data_ptr).data()), sizeof(float));
        target.push_back(t);
    }
    if (data.size() != target.size()){
        std::cout << "WARNING - size mismatch between data and targets of the replay buffer. Part of the information will be lost" <<
                    "\ndata size : " << data.size() << " | target size : " << target.size() << std::endl;
        uint32_t size = std::min(data.size(), target.size());
        data.resize(size, Array<float>{{input_size}, 0.0f});
        target.resize(size, Array<float>{{1}, 0.0f});
    }
    else if (target.size() == 0){
        std::cout << "WARNING - could not load replay buffer : empty files" << std::endl;
    } else {
        std::cout << "Successfully loaded replay buffer" << std::endl;
    }
}

void simulate(std::vector<Array<float>>* data, std::vector<Array<float>>* target,
    const algo& player){

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};
    
    algo player1{1, player};
    algo player2{-1, player};

    mv move;
    Array<float> state{{input_size}, 0.0f};

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

        game.play(move);
        data->emplace_back(toArray(game));

        // The target V value to learn is the result of running the algorithm and exploring the game tree.
        // By using the game tree, the algorithm improves the model prediction.
        // we use (-) the reward because the reward is from the perspective of the player that played the move,
        // and we want to learn with the input being from the perspective of the player that will play NEXT.
        target->emplace_back(std::vector<uint32_t>{1}, -reward);
    }
}

void collect_data(std::vector<Array<float>>& data, std::vector<Array<float>>& target,
    const algo& player, uint32_t games=100){

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

algo train(bool loadReplay, const algo& starting_player, uint32_t iteratons=100, uint32_t games=100,
    uint32_t replay_batch_size=10000, uint32_t GDepochs=10, uint32_t GDbatch_size=128, float lr=0.01f){
    
    algo player{1, starting_player};

    CheckersBoard game {conf::board_size[0], conf::board_size[1]};

    MSELoss loss {1};
    SGD optimizer(&player.model, &loss, 0.0f, train_conf::momentum);

    std::vector<Array<float>> data, target;
    if (loadReplay){
        loadReplayBuffer(data, target);
    }

    for(uint32_t i = 0; i < iteratons; i++){
        std::cout << "Iteration " << i+1 << "/" << iteratons << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        collect_data(data, target, player, games);

        std::cout << "\tData collected : " << data.size() << " samples" << std::endl;

        float average_game_length = data.size() / static_cast<float>(games * (i + 1));
        std::cout << "\tAverage game length : " << average_game_length << " moves" << std::endl;

        std::vector<Array<float>> batch_data, batch_target;
        fill_batch(data, target, batch_data, batch_target, replay_batch_size);
        optimizer.train(batch_data, batch_target, GDepochs, GDbatch_size, lr, 2, true, true);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::seconds iteration_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "\tIteration time : " << iteration_time.count() << " s" << std::endl;

        if (data.size() > train_conf::maxBufferSize){
            data.erase(data.begin(), data.begin() + (data.size() - train_conf::minBufferSize));
            target.erase(target.begin(), target.begin() + (target.size() - train_conf::minBufferSize));
        }
    }
    saveReplayBuffer(data, target);
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

    std::string load_str;
    std::cout << "load buffer ? (y/n) ";
    std::cin >> load_str;
    bool loadReplay = false;
    if (load_str == "y")
        loadReplay = true;
    
    algo player{1, load_file, 1000, 5.0f};

    auto start = std::chrono::high_resolution_clock::now();
    player = train(loadReplay, player, train_conf::iterations, train_conf::games,
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