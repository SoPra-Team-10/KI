//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include "AITools.h"
#include <SopraGameLogic/GameController.h>

namespace ai{
    int evalState(){
        int val = 0;

        //TODO: Make this a useful function

        return val;
    }

    auto computeOptimalPath(const gameModel::Position &start, const gameModel::Position &destination,
                            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position> {
        if(start == destination) {
            return {};
        }

        auto f = [&destination](){};

        std::deque<aiTools::SearchNode<gameModel::Position>> visited;
        std::vector<aiTools::SearchNode<gameModel::Position>> fringe;
        std::vector<gameModel::Position> ret;
        ret.reserve(gameController::getDistance(start, destination));
        int pathLen = 0;
        std::sort(fringe.begin(), fringe.end(), [&destination](const gameModel::Position &a, const gameModel::Position &b){
            return gameController::getDistance(a, destination) < gameController::getDistance(b, destination);
        });

        while(!fringe.empty()){

        }

        return ret;
    }
}
