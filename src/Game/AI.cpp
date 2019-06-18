//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include "AITools.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>

namespace ai{
    double evalState(const std::shared_ptr<gameModel::Environment> env, bool isLeft, bool goalScoredThisRound) {
        double val = 0;

        double valTeam1 = evalTeam(env->team1,
                                   env);

        double valTeam2 = evalTeam(env->team2,
                                   env);

        double valBludgers = evalBludgers(env);

        //Assume the KI plays left
        val = valTeam1 - valTeam2 + valBludgers;

        int bannedTeam1 = 0;
        int bannedTeam2 = 0;
        for(int i = 0; i < 7; i++){
            if(env->team1->getAllPlayers()[i]->isFined) bannedTeam1++;
            if(env->team2->getAllPlayers()[i]->isFined) bannedTeam2++;
        }
        if(bannedTeam1 >= 3){
            val -= 2000;
        }
        else if(goalScoredThisRound){
            val += bannedTeam1 * 150;
        }
        if(bannedTeam2 >= 3){
            val += 2000;
        }
        else if(goalScoredThisRound){
            val -= bannedTeam2 * 150;
        }

        int scoreDiff = env->team1->score - env->team2->score;
        if(scoreDiff < -30){
            val -= 700 + scoreDiff * 10;
        }
        else if(scoreDiff > 30){
            val += 700 + scoreDiff * 10;
        }
        else{
            val += scoreDiff * 15;
        }

        //If the KI does not play left, return negative val
        if(!isLeft){
            val = val * (-1);
        }

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
        int scoreDiff = 0;
        if(env->team1->hasMember(seeker)){
            scoreDiff = env->team1->score - env->team2->score;
        }
        else{
            scoreDiff = env->team2->score - env->team1->score;
        }
        if (seeker->isFined) return val;
        if(scoreDiff < -30){
            if(seeker->position == env->snitch->position){
                val = -1000;
            }
            else{
                val = 200 / gameController::getDistance(seeker->position, env->snitch->position);
            }
        }
        else {
            val = 1000 / gameController::getDistance(seeker->position, env->snitch->position) + 1;
        }

        return val;
    }

    double evalKeeper(const std::shared_ptr<gameModel::Keeper> keeper,
                      const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;
        int scoreDiff = 0;
        if(env->team1->hasMember(keeper)){
            scoreDiff = env->team1->score - env->team2->score;
        }
        else{
            scoreDiff = env->team2->score - env->team1->score;
        }
        if (keeper->isFined) return val;
        //If keeper has quaffle
        if (keeper->position == env->quaffle->position) {
            val += 100;
            if (env->isPlayerInOwnRestrictedZone(keeper)) {
                if (scoreDiff > -40) val += 20;
                if (scoreDiff >= 40) val += 500;
            } else {
                if(scoreDiff < -30){
                    val += 100 + getHighestGoalRate(env, keeper) * 10;
                }
                else if(scoreDiff > 30){
                    val += 200 + getHighestGoalRate(env, keeper);
                }
                else{
                    val += 100 + getHighestGoalRate(env, keeper) * 2;
                }
            }
        } else {

        }

        return val;
    }

    double evalChaser(const std::shared_ptr<gameModel::Chaser> chaser,
                      const std::shared_ptr<gameModel::Environment> env) {
        double val = 0;

        if (chaser->isFined) return val;

        int scoreDiff = 0;
        if(env->team1->hasMember(chaser)){
            scoreDiff = env->team1->score - env->team2->score;
        }
        else{
            scoreDiff = env->team2->score - env->team1->score;
        }
        //If Chaser holds quaffle
        if (chaser->position == env->quaffle->position) {
            if(scoreDiff < -30){
                val += 100 + getHighestGoalRate(env, chaser) * 10;
            }
            else if(scoreDiff > 30){
                val += 200 + getHighestGoalRate(env, chaser);
            }
            else{
                val += 100 + getHighestGoalRate(env, chaser) * 2;
            }
        }
        else{
            val += 100 / gameController::getDistance(chaser->position, env->quaffle->position);
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
        auto res = aiTools::aStarSearch(startNode, destinationNode, expandFunction, evalFunction);
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
