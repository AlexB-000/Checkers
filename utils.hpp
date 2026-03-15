#pragma once
#include <vector>
#include <algorithm>

struct Observer{
    virtual void update(int event) = 0;
};

struct Observable{
    std::vector<Observer*> observers;

    void notify(int event){
        for (auto obs: observers){
            obs->update(event);
        }
    }

    void add_observer(Observer* obs){
        observers.push_back(obs);
    }

    void remove_observer(Observer* obs){
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }
};