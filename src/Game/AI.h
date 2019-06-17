//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include "Game.hpp"
namespace aiTools {
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
}

#endif //KI_AI_H

