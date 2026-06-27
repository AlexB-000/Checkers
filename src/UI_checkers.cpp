#include <SFML/Graphics.hpp>

#include "conf.hpp"
#include "render.hpp"
#include "game.hpp"
#include "players/randomPlayer.hpp"
#include "players/humanPlayer.hpp"
#include "players/minimax.hpp"
#include "players/mcts.hpp"
#include "players/neuralNet.hpp"
#include "players/checkerZero.hpp"
#include "players/checkerOne.hpp"

int main(int argc, char* argv[]){
    std::shared_ptr<Player> player1;
    std::shared_ptr<Player> player2;

    uint next_arg = 2;
    if (std::string(argv[1]) == "human"){
        player1 = std::make_shared<HumanPlayer>(1);

    } else if (std::string(argv[1]) == "random"){
        player1 = std::make_shared<randomPlayer>(1);

    } else if (std::string(argv[1]) == "minimax"){
        player1 = std::make_shared<Minimax>(1, 8, true);

    } else if (std::string(argv[1]) == "mcts"){
        player1 = std::make_shared<MCTS>(1, 2000, 5.0, true);

    } else if (std::string(argv[1]) == "net"){
        player1 = std::make_shared<NeuralNetPlayer>(1, std::string(argv[2]));
        next_arg++;

    } else if (std::string(argv[1]) == "c0"){
        player1 = std::make_shared<CheckerZero>(1, std::string(argv[2]), 1000, 5.0);
        next_arg++;

    } else if (std::string(argv[1]) == "c1"){
        player1 = std::make_shared<CheckerOne>(1, std::string(argv[2]), 1000, 5.0);
        next_arg++;
    }

    if (std::string(argv[next_arg]) == "human"){
        player2 = std::make_shared<HumanPlayer>(-1);

    } else if (std::string(argv[next_arg]) == "random"){
        player2 = std::make_shared<randomPlayer>(-1);

    } else if (std::string(argv[next_arg]) == "minimax"){
        player2 = std::make_shared<Minimax>(-1, 8, true);

    } else if (std::string(argv[next_arg]) == "mcts"){
        player2 = std::make_shared<MCTS>(-1, 2000, 5.0, true);

    } else if (std::string(argv[next_arg]) == "net"){
        player2 = std::make_shared<NeuralNetPlayer>(-1, std::string(argv[next_arg + 1]));
        
    } else if (std::string(argv[next_arg]) == "c0"){
        player2 = std::make_shared<CheckerZero>(-1, std::string(argv[next_arg + 1]), 1000, 5.0);

    } else if (std::string(argv[next_arg]) == "c1"){
        player2 = std::make_shared<CheckerOne>(-1, std::string(argv[next_arg + 1]), 1000, 5.0);
    }

    auto window = sf::RenderWindow{sf::VideoMode({conf::window_size[0], conf::window_size[1]}), "Checkers", sf::Style::Close};

    CheckersGame game (&(*player1), &(*player2), conf::board_size[0], conf::board_size[1]);
    Render render(game, &window);

    // rendering
    window.clear();

    render.draw(window);

    window.display();

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {   
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
            else if (const auto& mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left){
                    sf::Vector2i localPosition = sf::Mouse::getPosition(window);
                    uint32_t col = localPosition.x / conf::square_size[0];
                    uint32_t r = localPosition.y / conf::square_size[1];
                    game.input(r, col);
                }
            }
        }

        game.make_move(conf::delay);

        // rendering
        window.clear();

        render.draw(window);

        window.display();
    }
}