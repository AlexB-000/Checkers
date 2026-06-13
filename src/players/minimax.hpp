#pragma once
#include "bot.hpp"

struct Minimax: virtual public Bot{
    int _depth;
    bool _prunning;

    Minimax(short inPlayer, int inDepth=4, bool inPrunning=true): Bot(inPlayer), _depth(inDepth), _prunning(inPrunning) {}
    Minimax(const Minimax& other): Bot(other.player), _depth(other._depth), _prunning(other._prunning) {}

    virtual float score_position(const CheckersBoard& board, short turn) {
        // won
        if ( board.winner == turn ) return INFINITY;
        // lost
        if ( board.winner == -turn ) return -INFINITY;

        int player1 = 0;
        int player2 = 0;
        for(uint16_t r = 0; r < board.size[0]; ++r){
            for(uint16_t c = 0; c < board.size[1]; ++c){
                if (board.board[r][c] == 1){
                    player1 ++;
                }
                else if (board.board[r][c] == 2){
                    player1 += 5;
                }
                else if (board.board[r][c] == -1){
                    player2 ++;
                }
                else if (board.board[r][c] == -2){
                    player2 += 5;
                }
            }
        }

        if (turn == 1) return player1 - player2;
        return player2 - player1;
    }

    bool terminal_node(const CheckersBoard& state) const {
        if (state.winner != 0) return true;
        return false;
    }

    mv minimax(const CheckersBoard& state, short player, float& score, int depth, float alpha, float beta,
        bool maximizingPlayer, bool pruning) {

        mv result;
        std::vector<mv> valid_moves = state.get_valid_moves(state.turn);
        
        if (depth == 0 || terminal_node(state)){
            score = score_position(state, player);
            return result;
        }

        if (valid_moves.size() == 0){
            score = 0;
            return result;
        }

        result = valid_moves[0];

        if (valid_moves.size() == 1){
            CheckersBoard stateCopy = state;
            stateCopy.play(result);
            
            score = score_position(stateCopy, player);
            return result;
        }

        if (maximizingPlayer){
            score = -INFINITY;
        }
        else{
            score = INFINITY;
        }

        for (const  mv& move: valid_moves){
            CheckersBoard stateCopy = state;
            stateCopy.play(move);

            float new_score = 0;
            minimax(stateCopy, player, new_score, depth-1, alpha, beta, !maximizingPlayer, pruning);

            if (maximizingPlayer){
                if (new_score > score){
                    score = new_score;
                    result = move;
                    alpha = std::max(alpha, score);
                }
            } else {
                if (new_score < score) {
                    score = new_score;
                    result = move;
                    beta = std::min(beta, score);
                }
            }
            if (pruning){
                if (alpha >= beta) break;
            }
        }

        return result;
    }

    mv move(const CheckersBoard& board) override {
        float score = 0;
        mv move = minimax(board, player, score, _depth, -INFINITY, INFINITY, true, _prunning);
        std::cout << "move score : " << score << "\n";
        return move;
    }
};