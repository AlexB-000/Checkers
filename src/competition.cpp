#include "competition.hpp"

void simulate(std::shared_ptr<Player> player1, std::shared_ptr<Player> player2, short* winner){
    CheckersBoard game {conf::board_size[0], conf::board_size[1]};
    mv move;
    // run the game
    uint32_t count = 0;
    bool running = true;
    while (running) {
        count++;

        // get the move and the assiociated reward that shoud be learned
        if (game.turn == 1){
            move = player1->move(game, false);
        } else {
            move = player2->move(game, false);
        }

        if (move.size() == 0 || game.checkWin() != 0 || count >= conf::moves_to_draw){
            running = false;
        }

        game.play(move);
    }
    *winner = game.winner;
}

std::vector<std::vector<std::vector<float>>> compete(const std::vector<std::shared_ptr<Player>>& players, uint32_t games){
    std::vector<std::vector<std::vector<float>>> winStats(players.size(), std::vector<std::vector<float>>(players.size(), {0.0f, 0.0f, 0.0f}));
    for (uint32_t i=0; i < players.size(); i++){
        for (uint32_t j=0; j < players.size(); j++){
            if (i==j) break;
            std::vector<short> winners (games, 0.0f);

            std::vector<std::thread> threads;
            threads.reserve(games);
            for (uint32_t g=0; g<games; g++){
                threads.emplace_back(&simulate, players[i]->copy(1), players[j]->copy(-1), &winners[g]);
            }
            for (std::thread& t : threads){
                t.join();
            }
            for (uint32_t g=0; g<games; g++){
                if (winners[g] == 1) winStats[i][j][0] += 1;
                else if (winners[g] == 0) winStats[i][j][1] += 1;
                else winStats[i][j][2] += 1;
            }
            winStats[i][j][0] /= games;
            winStats[i][j][1] /= games;
            winStats[i][j][2] /= games;
        }
    }
    return winStats;
}