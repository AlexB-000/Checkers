#include <chrono>
#include <random>
#include <thread>
#include "configuration.hpp"
#include "CheckersBoard.hpp"
#include "players/checkerZero.hpp"
#include "players/checkerOne.hpp"
#include "replayBuffer.hpp"

using algo = CheckerZero;

namespace train_conf{
    constexpr uint32_t iterations = 10;
    constexpr uint32_t games = 10;

    constexpr uint32_t algo_iterations = 1000;
    constexpr float exploration = 5.0f;
    
    constexpr uint32_t GDepochs = 5;
    constexpr uint32_t GDbatch_size = 64;
    constexpr float lr = 0.01f;
    constexpr float momentum = 0.9f;

    constexpr uint32_t bufferSize = 256000;
    constexpr uint32_t replay_batch_size = 64000;
};

enum class RewardMethod {
    tree_bootstrapping,
    raw_result
};

class Trainer {
    algo player;
    ReplayBuffer replayBuffer;

    RewardMethod r_type;

    float average_game_length;

    std::mutex mtx;

public:
    Trainer(RewardMethod reward_calculation, std::string load_file="")
        : r_type(reward_calculation),
          player(1, load_file, train_conf::algo_iterations, train_conf::exploration),
          replayBuffer(train_conf::bufferSize)
    {}
private:
    void fill_batch(std::vector<Array<float>>& batch_data, std::vector<Array<float>>& batch_target){
        uint32_t batch_size = train_conf::replay_batch_size;
        if (replayBuffer.size() == 0) return;
        if (replayBuffer.size() < train_conf::replay_batch_size) batch_size = replayBuffer.size();

        batch_data.clear();
        batch_target.clear();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, replayBuffer.size() - 1);

        for (uint32_t i = 0; i < batch_size; i++){
            uint32_t idx = dis(gen);
            Array<float> state {std::vector<uint32_t>{input_size}};
            float reward;
            replayBuffer.get(idx, state, reward);
            batch_data.emplace_back(state);
            batch_target.emplace_back(std::vector<uint32_t>{1}, reward);
        }
    }
    //MARK: Simulation
    void simulate(const algo& player){
        std::vector<Array<float>> states;
        std::vector<float> rewards;

        CheckersBoard game {conf::board_size[0], conf::board_size[1]};
        
        algo player1{1, player};
        algo player2{-1, player};

        mv move;

        float reward;
        uint32_t count = 0;

        if (r_type == RewardMethod::raw_result){
            // run the game
            while (count < conf::moves_to_draw && game.winner == 0){
                states.emplace_back(toArray(game));

                // get the move of the current player
                if (game.turn == 1){
                    move = player1.move(game, reward);
                } else {
                    move = player2.move(game, reward);
                }

                game.play(move);
                ++count;

                // draw if no move is possible
                if (move.size() == 0){
                    break;
                }
            }
            reward = game.winner * game.starter_player;

            for (uint32_t i=0; i < states.size(); i+=2){
                rewards.emplace_back(reward);
                if (i+1 < states.size()){
                    rewards.emplace_back(-reward);
                }
            }
        } else if (r_type == RewardMethod::tree_bootstrapping){
            // run the game
            while (count < conf::moves_to_draw && game.winner == 0) {
                // get the move and the assiociated reward that shoud be learned
                if (game.turn == 1){
                    move = player1.move(game, reward);
                } else {
                    move = player2.move(game, reward);
                }

                game.play(move);
                ++count;

                states.emplace_back(toArray(game));

                // The target V value to learn is the result of running the algorithm and exploring the game tree.
                // By using the game tree, the algorithm improves the model prediction.
                // we use (-) the reward because the reward is from the perspective of the player that played the move,
                // while we want to learn with the input after this move
                // (being from the perspective of the player that will play NEXT).
                rewards.emplace_back( -reward );

                // draw if no move is possible
                if (move.size() == 0){
                    break;
                }
            }
        }
        mtx.lock();
        average_game_length += count;
        for (uint32_t i = 0; i < states.size(); i++){
            replayBuffer.add(states[i], rewards[i]);
        }
        mtx.unlock();

    }

    void collect_data(){
        std::vector<std::thread> threads;
        threads.reserve(train_conf::games);

        for(uint32_t i = 0; i < train_conf::games; i++){
            threads.emplace_back(&Trainer::simulate, this, player);
        }
        for (std::thread& t : threads){
            t.join();
        }
    }
public:
    //MARK: train
    void train(std::string save_file="", std::string replay_load="", std::string replay_save=""){
        CheckersBoard game {conf::board_size[0], conf::board_size[1]};

        MSELoss loss {1};
        SGD optimizer(&player.model, &loss, 0.0f, train_conf::momentum);

        if (replay_load.size()){
            replayBuffer.load(replay_load);
        }

        for(uint32_t i = 0; i < train_conf::iterations; i++){
            average_game_length = 0.0f;

            std::cout << "Iteration " << i+1 << "/" << train_conf::iterations << std::endl;
            auto start = std::chrono::high_resolution_clock::now();

            collect_data();

            std::cout << "\tData collected : " << replayBuffer.size() << " samples" << std::endl;

            average_game_length /= static_cast<float>(train_conf::games * (i + 1));
            std::cout << "\tAverage game length : " << average_game_length << " moves" << std::endl;

            std::vector<Array<float>> batch_data, batch_target;
            fill_batch(batch_data, batch_target);
            optimizer.train(
                batch_data, batch_target,
                train_conf::GDepochs, train_conf::GDbatch_size, train_conf::lr,
                2, true, true
            );

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::seconds iteration_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);
            std::cout << "\tIteration time : " << iteration_time.count() << " s" << std::endl;
        }
        replayBuffer.save(replay_save);
        player.model.save(save_file);
    }
};

int main(int argc, char* argv[]){
    std::string load_file = "";
    std::string save_file = "./NeuralNet_Checkers";
    std::string replay_load = "";
    std::string replay_save = "./replay_buffer";
    RewardMethod r_method = RewardMethod::raw_result;
    uint32_t i = 0;
    while (i < argc) {
        if (std::string(argv[i]) == "--load"){
            load_file = std::string(argv[++i]);
        } else if (std::string(argv[i]) == "--save"){
            save_file = std::string(argv[++i]);

        } else if (std::string(argv[i]) == "--rload" || std::string(argv[i]) == "-rl"){
            replay_load = std::string(argv[++i]);
        } else if (std::string(argv[i]) == "--rsave" || std::string(argv[i]) == "-rs"){
            replay_save = std::string(argv[++i]);
        } else if (std::string(argv[i]) == "raw"){
            r_method = RewardMethod::raw_result;
        } else if (std::string(argv[i]) == "bootstrapping"){
            r_method = RewardMethod::tree_bootstrapping;
        }
        ++i;
    }

    if (r_method == RewardMethod::raw_result) {
        std::cout << "Choosed reward calculation : winner of the match" << std::endl;
    } else if (r_method == RewardMethod::tree_bootstrapping) {
        std::cout << "Choosed reward calculation : tree exploration output (bootstrapping)" << std::endl;
    }

    Trainer trainer(r_method, load_file);

    auto start = std::chrono::high_resolution_clock::now();
    trainer.train(save_file, replay_load, replay_save);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::seconds training_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    std::cout << "-- Training finished ---\n";
    std::cout << train_conf::iterations << " iterations : " << train_conf::games << " games / iteration\n";
    std::cout << "Total games played: " << train_conf::iterations * train_conf::games << std::endl;
    std::cout << "Training time: " << training_time.count() << " s" << std::endl;
}