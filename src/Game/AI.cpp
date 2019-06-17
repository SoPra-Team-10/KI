//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>

namespace aiTools {
    double evalState(const std::shared_ptr<gameModel::Environment> environment, bool isLeft) {
        double val = 0;

        //TODO: Make this a useful function
        gameModel::Position quafflePos;
        gameModel::Position snitchPos;
        std::shared_ptr<gameModel::Environment> env;
        int scoreDiff;

        env = environment->clone();
        quafflePos = environment->quaffle->position;
        snitchPos = environment->snitch->position;
        scoreDiff = env->team1->score - env->team2->score;

        double valTeam1 = evalTeam(env->team1,
                                   env);

        double valTeam2 = evalTeam(env->team2,
                                   env);

        double valBludgers = evalBludgers(env);

        if (isLeft) val = valTeam1 - valTeam2 + valBludgers;
        else val = valTeam2 - valTeam1 - valBludgers;


        return val;
    }

    double evalTeam(const std::shared_ptr<gameModel::Team> team,
                    const std::shared_ptr<gameModel::Environment> env) {

        double val = 0;

        val += evalSeeker(team->seeker, env);
        val += evalKeeper(team->keeper, env);
        val += evalChaser(team->chasers[0], env);
        val += evalChaser(team->chasers[1], env);
        val += evalChaser(team->chasers[2], env);

        return val;

    }

    double evalSeeker(const std::shared_ptr<gameModel::Seeker> seeker,
                      const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;
        int scoreDiff = env->team1->score - env->team2->score;
        if (seeker->isFined) return val;
        val = 1000 / gameController::getDistance(seeker->position, env->snitch->position) + 1;

        return val;
    }

    double evalKeeper(const std::shared_ptr<gameModel::Keeper> keeper,
                      const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;
        int scoreDiff = env->team1->score - env->team2->score;
        if (keeper->isFined) return val;
        if (keeper->position == env->quaffle->position) {
            val += 100;
            if (isKeeperInKeeperZone) {
                if (scoreDiff > -40) val += 100;
                if (scoreDiff >= 40) val += 500;
            } else {
                //TODO: rate his chance to score a goal
            }
        } else {

        }

        return val;
    }

    double evalChaser(const std::shared_ptr<gameModel::Chaser> chaser,
                      const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;
        int scoreDiff = env->team1->score - env->team2->score;
        if (chaser->isFined) return val;
        if (chaser->position == env->quaffle->position) {
            val += 100 + getHighestGoalRate(env, chaser);
        }
        return val;
    }

    double evalBludgers(const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;

        std::array<std::shared_ptr<gameModel::Bludger>, 2> bludgers = env->bludgers;
        std::shared_ptr<gameModel::Team> team1 = env->team1;
        std::shared_ptr<gameModel::Team> team2 = env->team2;

        for (int i = 0; i < 2; i++) {
            gameModel::Position bPos = bludgers[i]->position;

            //eval team1
            if (team1->keeper->position != bPos) val -= 10 /
                                                        (gameController::getDistance(bPos, team1->keeper->position) +
                                                         1);
            if (team1->seeker->position != bPos) val -= 20 /
                                                        (gameController::getDistance(bPos, team1->seeker->position) +
                                                         1);

            for (int j = 0; j < 3; j++) {
                if (team1->chasers[j]->position != bPos) val -= 10 / (gameController::getDistance(bPos,
                                                                                                  team1->chasers[j]->position) +
                                                                      1);
            }
            for (int j = 0; j < 2; j++) {
                if (team1->beaters[j]->position != bPos) val += 20 / (gameController::getDistance(bPos,
                                                                                                  team1->beaters[j]->position) +
                                                                      1);
            }

            //eval team2
            if (team2->keeper->position != bPos) val += 10 /
                                                        (gameController::getDistance(bPos, team2->keeper->position) +
                                                         1);
            if (team2->seeker->position != bPos) val += 20 /
                                                        (gameController::getDistance(bPos, team2->seeker->position) +
                                                         1);

            for (int j = 0; j < 3; j++) {
                if (team2->chasers[j]->position != bPos) val += 10 / (gameController::getDistance(bPos,
                                                                                                  team2->chasers[j]->position) +
                                                                      1);
            }
            for (int j = 0; j < 2; j++) {
                if (team2->beaters[j]->position != bPos) val -= 20 / (gameController::getDistance(bPos,
                                                                                                  team2->beaters[j]->position) +
                                                                      1);
            }
        }
        return val;
    }

    double getHighestGoalRate(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Player> actor) {
        double chance = 0;
        auto goalPos = env->getGoalsRight();

        if (env->team2->hasMember(actor)) {
            goalPos = env->getGoalsLeft();
        }

        for (int i = 0; i < 3; i++) {
            gameController::Shot shotSim = gameController::Shot(env, actor, env->quaffle, goalPos[i]);
            if (shotSim.successProb() > chance) chance = shotSim.successProb();
        }

        return chance;
    }
}

