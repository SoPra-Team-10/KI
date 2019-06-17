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

    auto computeOptimalPath(const std::shared_ptr<const gameModel::Player> &player, const gameModel::Position &destination,
                            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position> {
        if(player->position == destination) {
            return {};
        }

        auto expandFunction = [&env, &player](const aiTools::SearchNode<gameModel::Position> &node){
            std::vector<aiTools::SearchNode<gameModel::Position>> path;
            path.reserve(8);
            auto parent = std::make_shared<aiTools::SearchNode<gameModel::Position>>(node);
            for(const auto &pos : env->getAllLegalCellsAround(node.state, env->team1->hasMember(player))){
                path.emplace_back(pos, parent, node.pathCost + 1);
            }

            return path;
        };

        auto evalFunction = [&destination](const aiTools::SearchNode<gameModel::Position> &pos){
            int g = pos.parent.has_value() ? pos.parent.value()->pathCost + 1: 1;
            return g + gameController::getDistance(pos.state, destination);
        };

        aiTools::SearchNode<gameModel::Position> startNode(player->position, std::nullopt, 0);
        aiTools::SearchNode<gameModel::Position> destinationNode(destination, std::nullopt, 0);
        auto res = aiTools::aStarSearch<gameModel::Position>(startNode, destinationNode, expandFunction, evalFunction);
        std::vector<gameModel::Position> ret;
        if(res.has_value()){
            ret.reserve(res->pathCost + 1);
            ret.emplace_back(res->state);
            auto parent = res->parent;
            while(parent.has_value()){
                ret.emplace_back((*parent)->state);
                parent = (*parent)->parent;
            }
        }

        return ret;
    }
}
