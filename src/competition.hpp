#pragma once
#include <iostream>
#include <iomanip>
#include <memory>
#include <thread>
#include "configuration.hpp"
#include "players/player.hpp"

std::vector<std::vector<std::vector<float>>> compete(
    const std::vector<std::shared_ptr<Player>>& players,
    uint32_t games = 100
);