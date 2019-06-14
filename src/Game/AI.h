//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include <SopraGameLogic/GameModel.h>

namespace ai{
    int evalState();

    /**
     * Computes the optimal path from start to destination avoiding possible fouls using A*-search
     * @param start The Position to start
     * @param destination The desired gol Position
     * @param env The Environment to operate on
     * @return The optimal path as a list of Positions ordered from start to destination. Empty if start = destination or
     * when no path exists
     */
    auto computeOptimalPath(const gameModel::Position &start, const gameModel::Position &destination,
            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position>;
}

#endif //KI_AI_H

