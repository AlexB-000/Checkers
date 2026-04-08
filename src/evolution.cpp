#include <random>
#include <thread>
#include "conf.hpp"
#include "CheckersBoard.hpp"
#include "players/neuralNet.hpp"

Network crossover(Network& parent1, Network& parent2){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<short> dist(0, 1);
    
    Network child = parent1;
    std::vector<Array<float>*> parameters = child.get_parameters();
    std::vector<Array<float>*> parent2_parameters = parent2.get_parameters();

    for (uint32_t i = 0; i < parameters.size(); i++){
        Array<float>* param = parameters[i];
        for (uint32_t j = 0; j < (*param).data.size(); j++){
            if (dist(gen)){
                (*param).data[j] = (*parent2_parameters[i]).data[j];
            }
        }
    }
    return child;
}

void mutate(Network& individual, const float mutation_rate=0.01,
    const float new_value_mean=0.0f, const float new_value_devi = 1.0f){

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0, 1);

    std::normal_distribution<float> new_values_dist(new_value_mean, new_value_devi);
    
    std::vector<Array<float>*> parameters = individual.get_parameters();

    for (uint32_t i = 0; i < parameters.size(); i++){
        Array<float>* param = parameters[i];
        for (uint32_t j = 0; j < (*param).data.size(); j++){
            if (dist(gen) <= mutation_rate){
                (*param).data[j] = new_values_dist(gen);
            }
        }
    }
}

void repopulate(std::vector<NeuralNetPlayer> population, std::vector<float> scores, const float mutation_rate = 0.01f){
    std::vector<std::pair<NeuralNetPlayer*, float>> pairs;
    for (uint32_t i = 0; i < population.size(); i++){
        pairs.push_back({&population[i], scores[i]});
    }
    std::sort(pairs.begin(), pairs.end(), [](std::pair<NeuralNetPlayer*, float>right, std::pair<NeuralNetPlayer*, float>left){
        return left.second < right.second;
    });
    uint32_t half_size = population.size()/2;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, half_size-1);

    std::vector<NeuralNetPlayer> new_population;
    for (uint32_t i = 0; i < half_size; i++){
        new_population.push_back(*pairs[i].first);
        mutate(new_population[i].model, mutation_rate, 0.0f, 1.0f);
    }

    for (uint32_t i = 0; i < population.size() - half_size; i++){
        uint32_t parent1 = dist(gen);
        uint32_t parent2 = dist(gen);
        NeuralNetPlayer child(*pairs[parent1].first);
        child.model = crossover((*pairs[parent1].first).model, (*pairs[parent2].first).model);
        mutate(child.model, mutation_rate, 0.0f, 1.0f);
        new_population.push_back(child);
    }
    population = new_population;
}

void simulate(std::shared_ptr<CheckersBoard> game, NeuralNetPlayer player1, NeuralNetPlayer player2, float* score){
    std::vector<std::vector<uint16_t>> move;

    for (uint32_t i = 0; i < 500; i++){
        if (game->turn == 1){
            move = player1.move(*game, false);
        } else {
            move = player2.move(*game, false);
        }
        if (move.size() == 0){
            *score = game->checkWin();
            return;
        }
        game->play(move);
        if (game->checkWin() != 0){
            *score = game->checkWin();
            return;
        }
    }
    *score = 0.0f;
    return;
}

std::vector<float> evaluate(std::vector<NeuralNetPlayer> population){
    std::vector<float> scores (population.size(), 0.0f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, population.size()-1);

    std::vector<std::thread> threads;

    for (uint32_t i = 0; i < population.size(); i++){
        NeuralNetPlayer player1(1, population[i]);
        NeuralNetPlayer player2(-1, population[dist(gen)]);
        std::shared_ptr<CheckersBoard> game = std::make_shared<CheckersBoard>(conf::board_size[0], conf::board_size[1]);

        threads.push_back(std::thread(&simulate, game, player1, player2, &scores[i]));
    }
    for (std::thread& t : threads) {
        t.join();
    }
    return scores;
}

void initialize(std::vector<NeuralNetPlayer>& population, const uint32_t size, const NeuralNetPlayer& model,
    const float mutation_rate=0.01f){
    
    population.clear();
    for (uint32_t i = 0; i < size; i++){
        population.push_back(model);
        mutate(population[i].model, mutation_rate, 0.0f, 1.0f);
    }
}

void initialize(std::vector<NeuralNetPlayer>& population, const uint32_t size,
    const float mutation_rate=0.01f){
    
    population.clear();
    for (uint32_t i = 0; i < size; i++){
        population.push_back(NeuralNetPlayer(1));
        mutate(population[i].model, mutation_rate, 0.0f, 1.0f);
    }
}

NeuralNetPlayer evolution(std::vector<NeuralNetPlayer>& population, uint32_t generations=100, uint32_t individuals=1000,
    const float mutation_rate = 0.01f){

    for(uint32_t i = 0; i < generations; i++){
        std::cout << "generation " << i << "/" << generations << std::endl;

        std::vector<float> scores = evaluate(population);
        repopulate(population, scores, mutation_rate);

        std::cout << "  best score : " << *std::max_element(scores.begin(), scores.end()) << std::endl;
    }

    return population[0];
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
    
    std::vector<NeuralNetPlayer> population;

    if (load_file != ""){
        NeuralNetPlayer loaded_player{1, load_file};
        initialize(population, 1000, loaded_player, 0.001);
    } else {
        initialize(population, 1000, 0.001);
    }

    evolution(population, 500, 1000, 0.001);

    NeuralNetPlayer player = population[0];
    auto params = player.model.get_parameters();
    for (auto p: params){
        p->show();
    }
    player.model.save(save_file);
}