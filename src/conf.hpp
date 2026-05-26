#pragma once

namespace conf {
    constexpr unsigned int window_size[2] {700, 700};
    constexpr unsigned int board_size[2] {10, 10};
    constexpr unsigned int square_size[2] {window_size[0] / board_size[0], window_size[1] / board_size[1]};

    constexpr float pawn_size_ratio = 0.8;
    constexpr unsigned int pawn_radius = static_cast<unsigned int> (square_size[0] / 2 * pawn_size_ratio);
    constexpr unsigned int queen_radius = pawn_radius * 0.7;

    constexpr float delay = 0.1;

    constexpr uint32_t moves_to_draw = 300;
};