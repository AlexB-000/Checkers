#include "conf.hpp"
#include "competition.hpp"
#include "players/randomPlayer.hpp"
#include "players/humanPlayer.hpp"
#include "players/minimax.hpp"
#include "players/mcts.hpp"
#include "players/neuralNet.hpp"
#include "players/checkerZero.hpp"
#include "players/checkerOne.hpp"

void printWinStats(const std::vector<std::vector<std::vector<float>>>& winStats,
                    const std::vector<std::string>& algoNames, uint32_t spacing=30){
    std::string str = "Players";
    uint32_t len = 0;
    std::string::const_iterator s = str.begin();
    std::string spaces = "";
    for (uint32_t i=len; i<spacing; i++) spaces += " ";
    std::cout << str << spaces;

    for (uint32_t i=0; i < winStats.size(); i++){
        uint32_t len = 0;
        std::string::const_iterator s = algoNames[i].begin();
        while (*s) len += (*s++ & 0xc0) != 0x80;
        std::string spaces = "";
        for (uint32_t i=len; i<spacing; i++) spaces += " ";
        std::cout << algoNames[i] << spaces;
    }
    std::cout << std::endl << std::endl;
    std::cout << std::fixed << std::setprecision(3);


    std::string cell_spaces = "";
    for (uint32_t i=13 + 2 + 3 + 2; i<spacing; i++) spaces += " ";

    for (uint32_t i=0; i < winStats.size(); i++){
        for (uint32_t line=0; line<3; line++){
            uint32_t len = 0;
            if (line==1){
                std::string::const_iterator s = algoNames[i].begin();
                while (*s) len += (*s++ & 0xc0) != 0x80;
                std::cout << algoNames[i];
            }
            std::string spaces = "";
            for (uint32_t i=len; i<spacing; i++) spaces += " ";
            std::cout << spaces;

            for (uint32_t j=0; j < winStats.size(); j++){
                if (i==j) break;
                if (line==0){
                    std::cout << "Win rate :  ";
                } else if (line==1){
                    std::cout << "Draw rate : ";
                } else {
                    std::cout << "Lose rate : ";
                }
                std::cout << winStats[i][j][line] << " %" << cell_spaces;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

uint32_t strint(char input[]){
    uint32_t n = 0;
    char* iter = input;
    if (!*iter) return n;
    uint32_t len = 1;
    while (*++iter) len+=1;
    uint32_t pow=1;
    for (int32_t i=len-1; i>=0; --i){
        n += pow * (input[i] - 48);
        pow *= 10;
    }
    return n;
}

int main(int argc, char* argv[]){
    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::string> names;

    uint32_t games = 10;

    uint argIdx = 0;
    while (argIdx < argc){
        if (std::string(argv[argIdx]) == "human"){
            players.emplace_back(std::make_shared<HumanPlayer>(1));
            names.emplace_back("human");

        } else if (std::string(argv[argIdx]) == "random"){
            players.emplace_back(std::make_shared<randomPlayer>(1));
            names.emplace_back("random");

        } else if (std::string(argv[argIdx]) == "minimax"){
            players.emplace_back(std::make_shared<Minimax>(1, 8, true));
            names.emplace_back("minimax");

        } else if (std::string(argv[argIdx]) == "mcts"){
            players.emplace_back(std::make_shared<MCTS>(1, 2000, 5.0, true));
            names.emplace_back("mcts");

        } else if (std::string(argv[argIdx]) == "net"){
            players.emplace_back(std::make_shared<NeuralNetPlayer>(1, std::string(argv[++argIdx])));
            names.emplace_back("network");

        } else if (std::string(argv[argIdx]) == "c0"){
            players.emplace_back(std::make_shared<CheckerZero>(1, std::string(argv[++argIdx]), 1000, 5.0));
            names.emplace_back("Checker Zero");

        } else if (std::string(argv[argIdx]) == "c1"){
            players.emplace_back(std::make_shared<CheckerOne>(1, std::string(argv[++argIdx]), 1000, 5.0));
            names.emplace_back("Checker One");

        } else if (std::string(argv[argIdx]) == "games"){
            games = strint(argv[++argIdx]);
        }
        ++argIdx;
    }
    std::cout << "Starting competition - " << games << " games per pair of opponents" << std::endl;
    std::vector<std::vector<std::vector<float>>> stats = compete(players, games);
    printWinStats(stats, names, 30);
}