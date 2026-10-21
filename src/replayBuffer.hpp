#pragma once
#include <fstream>
#include "players/utils/netUtils.hpp"
#include "Arrays/ndarrays.hpp"

struct ReplayBuffer{
    std::vector<Array<float>> states;
    std::vector<float> rewards;
    uint32_t end = 0;
    const uint32_t max_size = 0;

    ReplayBuffer(uint32_t buffer_size) : max_size(buffer_size){
        states.reserve(buffer_size);
        rewards.reserve(buffer_size);
    }

    size_t size() const {
        return states.size();
    }

    void add(const Array<float>& state, float reward){
        if (states.size() < max_size){
            states.push_back(state);
            rewards.push_back(reward);
        } else {
            states[end] = state;
            rewards[end] = reward;
        }
        end = (end + 1) % max_size;
    }

    void get(uint32_t idx, Array<float>& state, float& reward){
        state = states[idx];
        reward = rewards[idx];
    }
    //MARK: save / load replay
    void save(const std::string directory) {
        std::ofstream stateFile(directory + "/states.bin", std::ios::binary);
        std::ofstream rewardFile(directory + "/rewards.bin", std::ios::binary);
        if (!stateFile) {
            std::cerr << "WARNING - Error opening file for saving replay buffer states" << std::endl;
        }
        if (!rewardFile) {
            std::cerr << "WARNING - Error opening file for saving replay buffer rewards" << std::endl;
        }
        if (!stateFile || !rewardFile) return ;

        for (const Array<float>& d : states) {
            stateFile.write(reinterpret_cast<const char*>((*d.data_ptr).data()), (*d.data_ptr).size() * sizeof(float));
        }
        for (const float& r : rewards) {
            rewardFile.write(reinterpret_cast<const char*>(&r), sizeof(float));
        }
        std::cout << "Successfully saved replay buffer" << std::endl;
    }

    void load(const std::string directory) {
        std::ifstream stateFile(directory + "/states.bin", std::ios::binary);
        std::ifstream rewardFile(directory + "/rewards.bin", std::ios::binary);
        if (!stateFile) {
            std::cerr << "WARNING - Error opening file for loading replay buffer states" << std::endl;
        }
        if (!rewardFile) {
            std::cerr << "WARNING - Error opening file for loading replay buffer rewards" << std::endl;
        }
        if (!stateFile || !rewardFile) return ;

        while (true) {
            Array<float> d(std::vector<uint32_t>{input_size});
            if (!stateFile.read(reinterpret_cast<char*>((*d.data_ptr).data()), sizeof(float)*input_size)) {
                break;
            }
            states.push_back(d);
        }
        while (true) {
            float r;
            if (!rewardFile.read(reinterpret_cast<char*>(&r), sizeof(float))) {
                break;
            }
            rewards.push_back(r);
        }
        if (states.size() != rewards.size()){
            std::cout << "WARNING - size mismatch between data and targets of the replay buffer. Part of the information will be lost" <<
                        "\ndata size : " << states.size() << " | reward size : " << rewards.size() << std::endl;
            uint32_t size = std::min(states.size(), rewards.size());
            states.resize(size, Array<float>{{input_size}, 0.0f});
            rewards.resize(size, 0.0f);
        }
        else if (rewards.size() == 0){
            std::cout << "WARNING - could not load replay buffer : empty files" << std::endl;
        } else {
            std::cout << "Successfully loaded replay buffer" << std::endl;
        }
    }
};