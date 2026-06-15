#pragma once
#include <iostream>
#include <thread>
#include <chrono>

#include "conf.hpp"
#include "utils.hpp"
#include "players/player.hpp"
#include "CheckersBoard.hpp"

struct CheckersGame: Observable {
    CheckersBoard checkBoard;
    Player* player1;
    Player* player2;

    std::vector<int32_t> selectedPawn = {-1, -1};
    std::vector<uint16_t> pos = {0, 0};

    std::vector<mv> played_move_options{};

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
            mv bot_move = player->move(checkBoard);
            // for (auto move: bot_move)
            //     std::cout << move[0] << " " << move[1] << " " << move[2] << " " << move[3] << "\n";
            if (bot_move.size()) play(bot_move, delay);
        }
        else {
            mv chosen_move = human_move();

            if (chosen_move.size()) play(chosen_move, delay);
        }
    }

    //MARK: play
    short play(mv move, const float delay = 0){
        if (move.size() == 0){
            checkBoard.turn = -checkBoard.turn;
            return checkBoard.winner;
        }

        // plays a move also using graphical interface
        uint16_t from_row = move[0];
        uint16_t from_col = move[1];
        short piece = checkBoard.board[from_row][from_col];
        for (uint16_t step = 2; step < move.size(); step+=2){
            uint16_t to_row = move[step];
            uint16_t to_col = move[step+1];
            
            int dr = to_row > from_row ? 1 : -1;
            int dc = to_col > from_col ? 1 : -1;
            int subSteps = std::abs(static_cast<int>(to_row)-static_cast<int>(from_row));

            for (uint16_t i = 0; i < subSteps; i++){
                checkBoard.board[from_row + i * dr][from_col + i * dc] = 0;
                checkBoard.board[from_row + (i+1) * dr][from_col + (i+1) * dc] = piece;
                notify(0);
                if (delay > 0.0f) {
                    std::this_thread::sleep_for(std::chrono::milliseconds( static_cast<int>(delay*1000) ));
                }
            }
            from_row = to_row;
            from_col = to_col;
        }

        checkBoard.checkPromotions();

        // Switch turn
        checkBoard.turn = -checkBoard.turn;

        return checkBoard.checkWin();
    }

    mv human_move(){

        if (selectedPawn == std::vector<int32_t>{-1, -1}) return {};

        mv move = {static_cast<uint16_t>(selectedPawn[0]), static_cast<uint16_t>(selectedPawn[1]),
                                      pos[0], pos[1]};

        // start of a new sequence
        if (played_move_options.size() == 0){
            std::vector<mv> valid_moves = checkBoard.get_valid_moves(checkBoard.turn);
            
            for (mv& v_move: valid_moves){
                if (v_move[0] == move[0] && v_move[1] == move[1] && v_move[2] == move[2] && v_move[3] == move[3]){
                    played_move_options.push_back(v_move);
                }
            }
            // move is not legal
            if (played_move_options.size() == 0) return {};
        }

        bool found = false;
        for (uint32_t i = 0; i < played_move_options.size(); i++){
            mv& move_opt = played_move_options[i];
            if (move_opt[0] == move[0] && move_opt[1] == move[1] && move_opt[2] == move[2] && move_opt[3] == move[3]) {
                // this sub-move is the end of a sequence (a full move)
                if (move_opt.size() == 4){
                    selectedPawn = {-1, -1};
                    played_move_options = {};
                    return move;
                }
                found = true;
                move_opt.erase(move_opt.begin(), move_opt.begin() + 4);
            } else {
                // this option is not possible with the requested move
                played_move_options.erase(played_move_options.begin() + i);
                i--;
            }
        }
        if (found) {
            // we set turn and selectedPawn for the continuation of this move
            checkBoard.turn = -checkBoard.turn;
            selectedPawn = {static_cast<int>(pos[0]), static_cast<int>(pos[1])};
            return move;
        }
        return {};
    }
};