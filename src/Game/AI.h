//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include "Game.hpp"
namespace ai {
    double evalState(const std::shared_ptr<gameModel::Environment> environment, bool isLeft);

    double evalTeam(const std::shared_ptr<gameModel::Team> team,
                    const std::shared_ptr<gameModel::Environment> env);

    double evalSeeker(const std::shared_ptr<gameModel::Seeker> seeker,
                      const std::shared_ptr<gameModel::Environment> env);

    double evalKeeper(const std::shared_ptr<gameModel::Keeper> keeper,
                      const std::shared_ptr<gameModel::Environment> env);

    double evalChaser(const std::shared_ptr<gameModel::Chaser> chaser,
                      const std::shared_ptr<gameModel::Environment> env);

    double evalBludgers(const std::shared_ptr<gameModel::Environment> env);

    double getHighestGoalRate(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Player> actor);

    /**
     * Computes the optimal path from start to destination avoiding possible fouls using A*-search
     * @param start The Position to start
     * @param destination The desired gol Position
     * @param env The Environment to operate on
     * @return The optimal path as a list of Positions ordered from start (vector::back) to destination (vector::front).
     * Start an destination are inclusivie. Empty if start = destination or when no path exists
     */
    auto computeOptimalPath(const std::shared_ptr<const gameModel::Player> &player, const gameModel::Position &destination,
                            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position>;
}

#endif //KI_AI_H

