#pragma once
#include <stdint.h>
#include <memory>
#include <string>
#include "CppAI/Cpp_AI.hpp"
#include "../../configuration.hpp"
#include "../../CheckersBoard.hpp"

constexpr uint16_t input_size = conf::board_size[0] * conf::board_size[1] * 2;

Network buildNetwork(std::string file = "");

Array<float> toArray(const CheckersBoard& state, short turn = 0);