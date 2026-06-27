#pragma once
#include "bot.hpp"
#include "neuralNet.hpp"
#include "utils/node.hpp"

//MARK: CheckerOne
struct CheckerOne: Bot{
    Network model;

    int _iterations;
    float _exploration_parameter;

    CheckerOne(const CheckerOne& other):
        Bot(other.player),
        model(other.model),
        _iterations(other._iterations),
        _exploration_parameter(other._exploration_parameter){}

    CheckerOne(short inPlayer, const CheckerOne& other): 
        Bot(inPlayer),
        model(other.model),
        _iterations(other._iterations),
        _exploration_parameter(other._exploration_parameter){}

    CheckerOne& operator=(const CheckerOne& other){
        player = other.player;
        model = other.model;
        _iterations = other._iterations;
        _exploration_parameter = other._exploration_parameter;
        return (*this);
    }

    CheckerOne(short inPlayer, std::string file = "",
        int in_interations=100, float in_exploration_parameter=0):
        Bot(inPlayer), model(buildNetwork(file)),
        _iterations(in_interations), _exploration_parameter(in_exploration_parameter){}

    std::shared_ptr<Player> copy(short inPlayer = 0) override { return std::make_shared<CheckerOne>(inPlayer, *this); }

    mv DL_tree_search(
        const Node& inRoot, float& reward,
        short inTurn, int iterations, float exploration_parameter
    ){
        /**
        Main function of CheckerOne, called whenever a move is needed.

        @param root root tree, starting point of the algorithm (board of the game
            at the moment a move is wanted)
        @param exploration_parameter: factor used in the CheckerOne

        @return: move to make
        */

        Node root = inRoot;

        if (root.valid_moves.size() == 0){
            reward = root.state.checkWin() * inTurn;
            return {};
        } else if (root.valid_moves.size() == 1){
            reward = 0;
            return root.valid_moves[0];
        }

        for (int i = 0; i < iterations; i++){
            short turn;
            Node& node = selection(root, inTurn, turn, exploration_parameter);
            float estimated_reward = estimation(node, turn);
            backpropagation(&node, estimated_reward, turn);
        }

        uint32_t index = best_child(root, 0);

        reward = root.children[index].reward / static_cast<float>(root.children[index].visits);
        return root.valid_moves[index];
    }

    Node& selection(Node& inNode, short turn, short& r_turn, float exploration_parameter){
        /**
        Expands the root node and takes the best child everytime until a winning state
        is reached. If a node is not fully explored, it is expanded and a child is returned.
        If it is fully explored, the best child of that node is taken.

        @param inNode: starting node
        @param turn: -1 or 1 according to which player plays next
        @param r_turn: turn of the result
        @param exploration_parameter: factor used for the CheckerOne algorithm
        */
        Node* node = &inNode;

        while ( node->valid_moves.size() && ! node->state.checkWin() ){
            if (!node->fully_explored()){
                r_turn = -turn;
                return expansion(*node);
            }
            else {
                node = &node->children[best_child(*node, exploration_parameter)];
                turn *= -1;
            }
        }
        r_turn = turn;
        return *node;
    }

    Node& expansion(Node& node){
        /**
        Add a child state to the node. Concretely, plays a move and adds
        it to the board of a newly created state, which is the child of the
        current node.

        @param node: current node to expand
        @return: a newly created child of the current node
        */

        mv move = node.valid_moves[node.children.size()];
        CheckersBoard new_state = node.state;
        new_state.play(move);

        node.add_child(new_state);

        return node.children[node.children.size()-1];
    }

    float estimation(Node& node, short turn){
        /**
        @param state: current state from which we should end up finding a winning situation
        @param turn: 1 or -1 depending on whose turn it is

        @return: a reward
        */        
        if (node.valid_moves.size() > 0){
            Array<float> state_action = toArray(node.state, turn);
            return model.forward(state_action)[0];
        }
        return node.state.checkWin() * turn;
    }

    void backpropagation(Node* inNode, float reward, short turn){
        /**
        Update the rewards of all the ancestors of a node. The reward is sometimes
        added and sometimes substracted from the current reward, since it takes
        into account the fact a winning move from the current player should be
        encouraged, but a winning move from the opponent should be discouraged.

        @param node: current node from which we start the backtracking
        @param reward: reward corresponding to that particular node
        */

       Node* node = inNode;

        while (node != nullptr) {
            node->visits += 1;
            if (turn == 1){
                node->reward = reward < node->reward ? reward : node->reward;
            } else {
                node->reward = reward > node->reward ? reward : node->reward;
            }
            node = (node->parent);
            turn *= -1;
        }
        return;
    }

    uint32_t best_child(Node& node, float exploration_parameter){
        /**
        Returns the best child of a node based on a scoring system proposed by Auer,
        Cesa-Bianchi and Fischer. This formula combines a term of exploration and
        a term of exploitation.

        @param node: node from which we want to find the best child
        @param factor: exploration parameter
        */
        float best_score = -INFINITY;
        std::vector<uint32_t> best_children_idx;

        for (uint32_t i=0; i < node.children.size(); i++){
            const Node& c = node.children[i];
            float exploitation = c.reward / static_cast<float>(c.visits);
            float exploration = std::sqrt(std::log2(static_cast<float>(node.visits)) / static_cast<float>(c.visits) );

            float score = exploitation + exploration_parameter * exploration;

            if (score == best_score)
                best_children_idx.push_back(i);

            else if (score > best_score){
                best_children_idx = {i};
                best_score = score;
            }
        }
        
        // random numbers generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, best_children_idx.size() - 1);
        
        return best_children_idx[dis(gen)];
    }

    mv move(const CheckersBoard& board, bool print) override {
        Node root = Node(board, nullptr);
        float reward;
        mv chosen_move = DL_tree_search(root, reward,
            player, _iterations, _exploration_parameter);
        if (print) std::cout << "Chosen move reward : " << reward << "\n";
        return chosen_move;
    }

    mv move(const CheckersBoard& board, float& reward) {
        Node root = Node(board, nullptr);
        return DL_tree_search(root, reward, player, _iterations, _exploration_parameter);
    }
};