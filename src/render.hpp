#pragma once
#include <SFML/Graphics.hpp>

#include "utils.hpp"
#include "configuration.hpp"
#include "game.hpp"

static const sf::Color Grid_White {150, 100, 60};
static const sf::Color Grid_Black {80, 40, 0};

static const std::vector<sf::Color> Player_White {{200, 200, 200}, {100, 200, 100}};
static const std::vector<sf::Color> Player_Black {{10, 10, 10}, {10, 60, 10}};

static const std::vector<sf::Color> Player_Queen_Out_White {{200, 200, 200}, {100, 200, 100}};
static const std::vector<sf::Color> Player_Queen_Out_Black {{10, 10, 10}, {10, 60, 10}};

static const std::vector<sf::Color> Player_Queen_In_White {{150, 150, 150}, {100, 150, 100}};
static const std::vector<sf::Color> Player_Queen_In_Black {{50, 50, 50}, {25, 75, 25}};

struct Render: Observer
{   
    Render(CheckersGame& inGame, sf::RenderWindow* inWindow): game (inGame), window(inWindow) { game.add_observer(this); }

    void update(int event) override {
        window->clear();
        draw(*window);
        window->display();
    }

    void draw(sf::RenderWindow &window){
        sf::RectangleShape square_shape;
        square_shape.setSize(sf::Vector2f(conf::square_size[0], conf::square_size[1]));

        sf::CircleShape pawn_shape;
        pawn_shape.setRadius(conf::pawn_radius);
        pawn_shape.setOrigin({conf::pawn_radius, conf::pawn_radius});

        sf::CircleShape queen_shape;
        queen_shape.setRadius(conf::queen_radius);
        queen_shape.setOrigin({conf::queen_radius, conf::queen_radius});

        sf::Color square_color;
        sf::Color pawn_color;
        sf::Color queenColor;

        bool selected;

        for (uint16_t i = 0; i < game.checkBoard.size[0]; i++){
            for (uint16_t j = 0; j < game.checkBoard.size[1]; j++){

                if (i == game.selectedPawn[0] && j == game.selectedPawn[1]) selected = 1;
                else selected = 0;

                square_shape.setPosition({static_cast<float>(j * conf::square_size[0]), static_cast<float>(i * conf::square_size[1])});

                if (!((i+j) % 2)) square_color = Grid_White;
                else square_color = Grid_Black;

                square_shape.setFillColor(square_color);

                window.draw(square_shape);

                if (game.checkBoard.board[i][j] != 0){

                    pawn_shape.setPosition({static_cast<float>(j * conf::square_size[0] + conf::square_size[0]/2),
                                            static_cast<float>(i * conf::square_size[1] + conf::square_size[1]/2)});

                    if (game.checkBoard.board[i][j] == game.checkBoard.starter_player)
                        pawn_color = Player_White[selected];
                    else if (game.checkBoard.board[i][j] == game.checkBoard.starter_player*-1)
                        pawn_color = Player_Black[selected];

                    else if (game.checkBoard.board[i][j] == game.checkBoard.starter_player * 2)
                        pawn_color = Player_Queen_Out_White[selected];
                    else if (game.checkBoard.board[i][j] == game.checkBoard.starter_player * -2)
                        pawn_color = Player_Queen_Out_Black[selected];

                    pawn_shape.setFillColor(pawn_color);

                    window.draw(pawn_shape);
                    
                    if (std::abs(game.checkBoard.board[i][j]) != 2) continue;

                    queen_shape.setPosition({static_cast<float>(j * conf::square_size[0] + conf::square_size[0]/2),
                                             static_cast<float>(i * conf::square_size[1] + conf::square_size[1]/2)});

                    if (game.checkBoard.board[i][j] == game.checkBoard.starter_player * 2)
                        queenColor = Player_Queen_In_White[selected];
                    else if (game.checkBoard.board[i][j] == game.checkBoard.starter_player * -2)
                        queenColor = Player_Queen_In_Black[selected];
                    
                    queen_shape.setFillColor(queenColor);
                    window.draw(queen_shape);
                }

            }
        }
    }

    CheckersGame& game;
    sf::RenderWindow* window;
};