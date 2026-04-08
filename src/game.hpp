#pragma once
#include <iostream>
#include <thread>
#include <chrono>

#include "utils.hpp"
#include "players/player.hpp"
#include "CheckersBoard.hpp"

bool contains(const std::vector<std::vector<std::vector<uint16_t>>>& vector, const std::vector<std::vector<uint16_t>>& target){
    for (const std::vector<std::vector<uint16_t>>& element : vector){
        if (element == target){
            return true;
        }
    }
    return false;
};

struct CheckersGame: Observable {
    CheckersBoard checkBoard;
    Player* player1;
    Player* player2;

    std::vector<int32_t> selectedPawn = {-1, -1};
    std::vector<uint16_t> pos = {0, 0};

    std::vector<std::vector<std::vector<uint16_t>>> played_move_options{};

    CheckersGame (Player* playerA, Player* playerB, uint16_t rows = 10, uint16_t cols = 10):
        player1(playerA), player2(playerB), checkBoard(rows, cols) {}

    void input(uint16_t r, uint16_t col){
        pos = {r, col};

        if (checkBoard.board[r][col] == checkBoard.turn ||
            checkBoard.board[r][col] == checkBoard.turn*2 ){

            selectedPawn[0] = r;
            selectedPawn[1] = col;
        }
    }

    //MARK: make_move
    void make_move(const float delay = 0){
        Player* player;
        if (checkBoard.turn == 1) player = player1;
        else player = player2;

        if (player->type){
            std::vector<std::vector<uint16_t>> bot_move = player->move(checkBoard);
            // for (auto move: bot_move)
            //     std::cout << move[0] << " " << move[1] << " " << move[2] << " " << move[3] << "\n";
            if (bot_move.size()) play(bot_move, delay);
        }
        else {
            std::vector<std::vector<uint16_t>> chosen_move = human_move();

            if (chosen_move.size()) play(chosen_move, delay);
        }
    }

    //MARK: play
    short play(std::vector<std::vector<uint16_t>> move, const float delay = 0){
        // plays a move also using graphical interface
        for (std::vector<uint16_t> & step: move){
            const uint16_t from_row = step[0];
            const uint16_t from_col = step[1];
            const uint16_t to_row = step[2];
            const uint16_t to_col = step[3];

            short piece = checkBoard.board[from_row][from_col];
            checkBoard.board[from_row][from_col] = 0;
            
            int dr = to_row > from_row ? 1 : -1;
            int dc = to_col > from_col ? 1 : -1;
            int subSteps = std::abs(static_cast<int>(to_row)-static_cast<int>(from_row));

            for (uint32_t i = 0; i < subSteps; i++){
                checkBoard.board[from_row + i * dr][from_col + i * dc] = 0;
                checkBoard.board[from_row + (i+1) * dr][from_col + (i+1) * dc] = piece;
                notify(0);
                if (delay > 0.0f) {
                    std::this_thread::sleep_for(std::chrono::milliseconds( static_cast<int>(delay*1000) ));
                }
            }
        }

        checkBoard.checkPromotions();

        // Switch turn
        checkBoard.turn = -checkBoard.turn;

        return checkBoard.checkWin();
    }

    std::vector<std::vector<uint16_t>> human_move(){

        if (selectedPawn == std::vector<int32_t>{-1, -1}) return {};

        std::vector<uint16_t> move = {static_cast<uint16_t>(selectedPawn[0]), static_cast<uint16_t>(selectedPawn[1]),
                                      pos[0], pos[1]};

        if (played_move_options.size() == 0){
            std::vector<std::vector<std::vector<uint16_t>>> valid_moves = checkBoard.get_valid_moves(checkBoard.turn);
            
            for (std::vector<std::vector<uint16_t>>& v_move: valid_moves){
                if (v_move[0] == move){
                    played_move_options.push_back(v_move);
                }
            }
            // move is not legal
            if (played_move_options.size() == 0) return {};
        }

        bool found = false;
        for (std::vector<std::vector<uint16_t>>& move_opt: played_move_options){
            if (move_opt[0] == move) {
                // this sub-move is the end of a sequence (a full move)
                if (move_opt.size() == 1){
                    selectedPawn = {-1, -1};
                    played_move_options = {};
                    return {move};
                }
                found = true;
                move_opt.erase(move_opt.begin());
            }
        }
        if (found) {
            for (uint32_t i = 0; i < played_move_options.size(); i++){
                if (played_move_options[i][0] != move) {
                    // this option is not possible with the requested move
                    played_move_options.erase(played_move_options.begin() + i);
                    i--;
                }
            }
            // we set turn and selectedPawn for the continuation of this move
            checkBoard.turn = -checkBoard.turn;
            selectedPawn = {static_cast<int>(pos[0]), static_cast<int>(pos[1])};
            return {move};
        }
        return {};
    }
};