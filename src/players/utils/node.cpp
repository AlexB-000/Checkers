#include "node.hpp"

std::mt19937 Node::gen{std::random_device{}()};

Node::Node(const CheckersBoard& inState, Node* inParent): state(inState), parent(inParent) {
    valid_moves = state.get_valid_moves(state.turn);
    std::shuffle(valid_moves.begin(), valid_moves.end(), gen);
}

void Node::update_parents(){
    for ( Node& child: children ){
        child.parent = this;
        child.update_parents();
    }
}

void Node::add_child(const CheckersBoard& child_state){
    children.push_back( Node(child_state, this) );
}

bool Node::fully_explored(){
    if (children.size() == valid_moves.size())
        return true;
    return false;
}