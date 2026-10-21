#pragma once
#include <vector>
#include <random>
#include <algorithm>

#include "../../configuration.hpp"
#include "../../CheckersBoard.hpp"

struct Node{
    /**
    This class is used to represent nodes of the tree of boards used during
    Monte-Carlo Tree Search.
    */

    unsigned int visits = 0;
    float reward = 0;

    CheckersBoard state;
    Node* parent;
    std::vector<Node> children;
    std::vector<mv> valid_moves;

    static std::mt19937 gen;

    Node(){}

    Node(const CheckersBoard& inState, Node* inParent);

    void update_parents();

    /**
    Add a child to the current node.

    @param child_state: state of the child to add
    @param move: move to do to get to the newly added child
    */
    void add_child(const CheckersBoard& child_state);

    /**
    Checks if the node is fully explored (which means we can not add
    any more children to this node)

    @return True of False depending on if it is fully epxlored or not
    */
    bool fully_explored();
};