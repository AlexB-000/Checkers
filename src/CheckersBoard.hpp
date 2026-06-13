#pragma once
#include <vector>
#include <string>
#include <random>

struct CheckersBoard
{
    CheckersBoard(uint16_t rows = 10, uint16_t cols = 10){
        size[0] = rows;
        size[1] = cols;
        reset();
    }

    void reset(){
        board.clear();
        board.resize(size[0], std::vector<short>(size[1], 0));
        place_pawns();

        //random numbers generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1);

        starter_player = dis(gen) ? 1 : -1;
        turn = starter_player;

        winner = 0;
    }

    void place_pawns(){
        for(uint16_t r = 0; r < size[0]; ++r){
            for(uint16_t c = 0; c < size[1]; ++c){

                if((r + c) % 2 == 1){
                    if(r < size[0]/2 - 1){
                        board[r][c] = 1; // Player 1 pawn
                    } else if(r > size[0]/2){
                        board[r][c] = -1; // Player 2 pawn
                    }
                }
            }
        }
    }

    short checkWin(){
        winner = 0;

        bool player1_has_pawns = false;
        bool player2_has_pawns = false;

        for(const auto& row : board){
            for(const auto& cell : row){
                if(cell > 0){
                    player1_has_pawns = true;
                } else if(cell < 0){
                    player2_has_pawns = true;
                }
                if (player1_has_pawns && player2_has_pawns){
                    winner = 0;
                    return 0; // Both players still have pawns
                }
            }
        }
        if(!player1_has_pawns){
            winner = -1;
        } else if(!player2_has_pawns){
            winner = 1;
        }
        return winner;
    }

    void checkPromotions(){
        for(uint16_t c = 0; c < size[1]; ++c){
            if(board[0][c] == -1){
                board[0][c] = -2; // Promote to queen
            }
            if(board[size[0]-1][c] == 1){
                board[size[0]-1][c] = 2; // Promote to queen
            }
        }
    }

    //MARK: play
    short play(mv move){
        uint16_t from_row = move[0];
        uint16_t from_col = move[1];
        short piece = board[from_row][from_col];
        for (uint16_t step = 2; step < move.size(); step+=2){
            uint16_t to_row = move[step];
            uint16_t to_col = move[step+1];
            
            int dr = to_row > from_row ? 1 : -1;
            int dc = to_col > from_col ? 1 : -1;
            uint subSteps = std::abs(static_cast<int>(to_row)-static_cast<int>(from_row));

            for (uint16_t i = 0; i < subSteps; i++){
                board[from_row + i * dr][from_col + i * dc] = 0;
            }
            from_row = to_row;
            from_col = to_col;
        }
        board[from_row][from_col] = piece;

        checkPromotions();

        // Switch turn
        turn = -turn;

        return checkWin();
    }

//MARK: MOVES
    std::vector<mv> valid_simple_moves(uint16_t row, uint16_t col) const {
        std::vector<mv> valid_moves;

        short piece = board[row][col];
        if(piece == 0) return valid_moves; // No piece at the given position

        bool regular_pawn = std::abs(piece) == 1;   

        for (short i = -1; i <= 1; i += 2){
            if (regular_pawn){
                if ( (piece > 0 && i == -1) || (piece < 0 && i == 1) ) continue;
            }

            for (short j = -1; j <= 1; j += 2){

                for (uint16_t step = 1; step < std::max(size[0], size[1]) ; ++step) {
                    
                    if (regular_pawn && step > 1) break;

                    uint16_t new_row = row + i*step;
                    uint16_t new_col = col + j*step;

                    // Check if the new position is within bounds
                    if(new_row >= size[0] || new_row < 0 || new_col >= size[1] || new_col < 0) break;

                    if(board[new_row][new_col] == 0){
                        valid_moves.push_back(mv {row, col, new_row, new_col});

                    } else break; // Stop if we hit a non-empty square
                }
            }
        }

        return valid_moves;
    }

   std::vector<mv> valid_simple_moves(short player) const {
        std::vector<mv> all_valid_moves;

        for(uint16_t r = 0; r < size[0]; ++r){
            for(uint16_t c = 0; c < size[1]; ++c){
                if((player == 1 && board[r][c] > 0) || (player == -1 && board[r][c] < 0)){
                    auto moves = valid_simple_moves(r, c);
                    for(const auto& move : moves){
                        all_valid_moves.push_back(move);
                    }
                }
            }
        }
        return all_valid_moves;
    }

    //MARK: get_eat_moves
    std::vector<mv> get_eat_moves(uint16_t row, uint16_t col,
        std::vector<std::vector<short>> in_board, std::vector<int>& move_stats) const {
        int best_move = 0;
        std::vector<mv> eat_moves;

        short piece = in_board[row][col];
        if(piece == 0) return eat_moves; // No piece at the given position

        bool regular_pawn = std::abs(piece) == 1;

        for (short i = -1; i <= 1; i += 2){
            for (short j = -1; j <= 1; j += 2){
                bool pawn = false;

                uint16_t inter_row;
                uint16_t inter_col;
                
                for (uint16_t step = 1; step < std::max(size[0], size[1]) ; ++step) {
                    if (regular_pawn && step > 2) break;

                    uint16_t target_row = row + i*step;
                    uint16_t target_col = col + j*step;

                    // Check if the target position is within bounds
                    if(target_row >= size[0] || target_row < 0 || target_col >= size[1] || target_col < 0) break;

                    if (in_board[target_row][target_col] == 0){
                        if (!pawn) continue;

                        mv move = {row, col, target_row, target_col};

                        std::vector<std::vector<short>> next_board = in_board;
                        next_board[row][col] = 0;
                        next_board[inter_row][inter_col] = 0;
                        next_board[target_row][target_col] = in_board[row][col];

                        std::vector<int> next_moves_stats;
                        std::vector<mv> next_moves =
                            get_eat_moves(target_row, target_col, next_board, next_moves_stats);

                        if (next_moves.size() == 0){
                            if (best_move < 1) {
                                best_move = 1;
                                eat_moves = {move};
                                move_stats = {1};
                            } else if (best_move == 1) {
                                eat_moves.push_back(move);
                                move_stats.push_back(1);
                            }
                        } else {
                            for (uint32_t i = 0; i < next_moves.size(); i++){
                                const mv& m = next_moves[i];

                                mv complete_move = move;
                                complete_move.insert(complete_move.end(), m.begin()+2, m.end());
                                
                                int eat_num = 1 + next_moves_stats[i];
                                if (best_move < eat_num) {
                                    best_move = eat_num;
                                    eat_moves = {complete_move};
                                    move_stats = {eat_num};
                                }
                                else if (best_move == eat_num) {
                                    eat_moves.push_back(complete_move);
                                    move_stats.push_back(eat_num);
                                }
                            }
                        }

                        continue;
                    }
                    
                    if ( (in_board[target_row][target_col] > 0) == !(piece > 0) ) {
                        // no place to jump after the first pawn
                        if (pawn) break;
                        inter_row = target_row;
                        inter_col = target_col;
                        pawn = true;
                    }
                    else break; // friend pawn
                }
            }
        }

        return eat_moves;
    }

    std::vector<mv> get_eat_moves(uint16_t row, uint16_t col) const {
        std::vector<int> move_stats;
        return get_eat_moves(row, col, board, move_stats);
    }

    std::vector<mv> get_eat_moves(short player) const {
        std::vector<mv> eat_moves;

        for(uint16_t r = 0; r < size[0]; ++r){
            for(uint16_t c = 0; c < size[1]; ++c){
                if((player == 1 && board[r][c] > 0) || (player == -1 && board[r][c] < 0)){
                    auto moves = get_eat_moves(r, c);
                    for(const auto& move : moves){
                        eat_moves.push_back(move);
                    }
                }
            }
        };
        return eat_moves;
    }

    std::vector<mv> get_valid_moves(short player) const {
        std::vector<mv> valid_moves = get_eat_moves(player);
        if (valid_moves.size()) return valid_moves;

        else return valid_simple_moves(player);
    }
    
    uint16_t size[2] {0, 0}; // rows, cols

    short turn = 1; // 1 or -1
    short starter_player = 1; // 1 or -1
    short winner = 0; // 0 - no winner, 1 - player 1 wins, -1 - player 2 wins

    std::vector<std::vector<short>> board;
};
