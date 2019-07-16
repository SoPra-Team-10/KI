//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/conversions.h>
#include <SopraAITools/AITools.h>
#include <iostream>

namespace ai{

    double evalState(const std::shared_ptr<const gameModel::Environment> &env, gameModel::TeamSide mySide,
                     bool goalScoredThisRound) {

        constexpr auto disqPenalty = 2000;
        constexpr auto unbanDiscountFactor = 150;
        constexpr auto maxBanCount = 3;
        constexpr auto farBehindPenalty = 3000;
        constexpr auto leadBonus = 1000;
        constexpr auto scoreDiffFarBehindDiscountFactor = 300;
        constexpr auto scoreDiffDiscountFactor = 200;
        constexpr auto scoreDiffInLeadDiscountFactor = 100;

        double val = 0;
        auto localEnv = env->clone();
        auto valTeam1 = evalTeam(localEnv->getTeam(gameModel::TeamSide::LEFT), localEnv);
        auto valTeam2 = evalTeam(localEnv->getTeam(gameModel::TeamSide::RIGHT), localEnv);
        auto valBludgers = evalBludgers(localEnv, mySide);

        //Assume the KI plays left
        val = valTeam1 - valTeam2;
        val += mySide == gameModel::TeamSide::LEFT ? valBludgers : -valBludgers;

        int bannedTeam1 = localEnv->getTeam(gameModel::TeamSide::LEFT)->numberOfBannedMembers();
        int bannedTeam2 = localEnv->getTeam(gameModel::TeamSide::RIGHT)->numberOfBannedMembers();

        if(bannedTeam1 >= maxBanCount) {
            val -= disqPenalty;
        } else if(goalScoredThisRound) {
            val += bannedTeam1 * unbanDiscountFactor;
        }

        if(bannedTeam2 >= maxBanCount) {
            val += disqPenalty;
        } else if(goalScoredThisRound) {
            val -= bannedTeam2 * unbanDiscountFactor;
        }

        int scoreDiff = localEnv->getTeam(gameModel::TeamSide::LEFT)->score -
                            localEnv->getTeam(gameModel::TeamSide::RIGHT)->score;
        if(scoreDiff < -gameController::SNITCH_POINTS) {
            val += -farBehindPenalty + scoreDiff * scoreDiffFarBehindDiscountFactor;
        } else if(scoreDiff > gameController::SNITCH_POINTS) {
            val += leadBonus + scoreDiff * scoreDiffInLeadDiscountFactor;
        } else {
            val += scoreDiff * scoreDiffDiscountFactor;
        }

        //If the KI does not play left, return negative val
        return mySide == gameModel::TeamSide::LEFT ? val : -val;
    }

    double evalTeam(const std::shared_ptr<const gameModel::Team> &team, const std::shared_ptr<gameModel::Environment> &env) {
        double val = 0;
        val += evalSeeker(team->seeker, env);
        val += evalKeeper(team->keeper, env);
        for(const auto &chaser : team->chasers){
            val += evalChaser(chaser, env);
        }

        return val;

    }

    double evalSeeker(const std::shared_ptr<const gameModel::Seeker> &seeker,
                      const std::shared_ptr<const gameModel::Environment> &env) {
        constexpr auto optimalPathThreshold = 5;
        constexpr auto gameLosePenalty = 2000;
        constexpr auto baseSnitchdistanceDiscount = 200.0;
        constexpr auto winSnitchDistanceDiscount = 1000.0;
        constexpr auto nearCenterDiscount = 10.0;
        constexpr auto baseVal = 500;
        constexpr auto knockoutPenalty = 500;

        constexpr auto centerX = 8;
        constexpr auto centerY = 6;

        double val = 0;

        if(seeker->isFined) {
            return val;
        } else {
            val += baseVal;
        }
        if(seeker->knockedOut){
            val -= knockoutPenalty;
        }

        int scoreDiff = 0;
        if(env->team1->hasMember(seeker)) {
            scoreDiff = env->team1->score - env->team2->score;
        } else {
            scoreDiff = env->team2->score - env->team1->score;
        }

        if(env->snitch->exists){
            if(scoreDiff < -gameController::SNITCH_POINTS){
                if(seeker->position == env->snitch->position){
                    val = -gameLosePenalty * env->config.getGameDynamicsProbs().catchSnitch;
                }
                else{
                    val = baseSnitchdistanceDiscount / gameController::getDistance(seeker->position, env->snitch->position);
                }
            }
            else {
                if(gameController::getDistance(seeker->position, env->snitch->position) > optimalPathThreshold) {
                    val = winSnitchDistanceDiscount / (gameController::getDistance(seeker->position, env->snitch->position) + 1);
                } else {
                    val = winSnitchDistanceDiscount / (aiTools::computeOptimalPath(seeker, env->snitch->position, env).size() + 1);
                }
            }
        } else{
            val += nearCenterDiscount / (gameController::getDistance(seeker->position, gameModel::Position(centerX, centerY)) + 1);
        }

        return val;
    }

    double
    evalKeeper(const std::shared_ptr<gameModel::Keeper> &keeper, const std::shared_ptr<gameModel::Environment> &env) {
        constexpr auto holdsQuaffleBaseDiscount = 450;
        constexpr auto keeperBonusEvenWinChance = 20;
        constexpr auto keeperBonusHighWinChance = 500;
        constexpr auto goalChanceDiscountFactorBehind = 1000;
        constexpr auto goalChanceDiscountFactorInLead = 200;
        constexpr auto goalChanceDiscountFactorEven = 500;
        constexpr auto baseQuaffleDistanceDiscount = 150.0;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 500;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 100;
        constexpr auto goalPotentialChanceDiscountFactorEven = 200;
        constexpr auto baseVal = 450;
        constexpr auto keeperBonusPotentialEvenWinChance = 10;
        constexpr auto keeperBonusPotentialHighWinChance = 200;
        constexpr auto knockoutPenalty = 500;

        double val = 0;
        int scoreDiff = 0;
        if(env->team1->hasMember(keeper)) {
            scoreDiff = env->team1->score - env->team2->score;
        } else {
            scoreDiff = env->team2->score - env->team1->score;
        }

        if (keeper->isFined || keeper->knockedOut) {
            return val;
        } else{
            val += baseVal;
        }
        if(keeper->knockedOut){
            val -= knockoutPenalty;
        }

        //If keeper has quaffle
        if (keeper->position == env->quaffle->position) {
            val += holdsQuaffleBaseDiscount;
            if (env->isPlayerInOwnRestrictedZone(keeper)) {
                if (scoreDiff >= -gameController::SNITCH_POINTS) {
                    val += keeperBonusEvenWinChance;
                } else if (scoreDiff > gameController::SNITCH_POINTS) {
                    val += keeperBonusHighWinChance;
                }
            } else {
                if(scoreDiff < -gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, keeper) * goalChanceDiscountFactorBehind;
                } else if(scoreDiff > gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, keeper) * goalChanceDiscountFactorInLead;
                } else {
                    val += getHighestGoalRate(env, keeper) * goalChanceDiscountFactorEven;
                }
            }
        } else {
            if(teamHasQuaffle(env, keeper)) {
                if(scoreDiff < -gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorBehind;
                } else if(scoreDiff > gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorInLead;
                } else {
                    val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorEven;
                }

                if (env->isPlayerInOwnRestrictedZone(keeper)) {
                    if(scoreDiff > gameController::SNITCH_POINTS) {
                        val += keeperBonusPotentialHighWinChance;
                    } else if(scoreDiff >= -gameController::SNITCH_POINTS) {
                        val += keeperBonusPotentialEvenWinChance;
                    }
                }
            } else {
                val += baseQuaffleDistanceDiscount / gameController::getDistance(keeper->position, env->quaffle->position);
            }

        }

        return val;
    }

    double evalChaser(const std::shared_ptr<gameModel::Chaser> &chaser,
                      const std::shared_ptr<gameModel::Environment> &env) {
        constexpr auto goalChanceDiscountFactorBehind = 1000;
        constexpr auto goalChanceDiscountFactorInLead = 200;
        constexpr auto goalChanceDiscountFactorEven = 600;
        constexpr auto baseQuaffleDistanceDiscount = 150.0;
        constexpr auto holdsQuaffleBaseDiscount = 450;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 500;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 100;
        constexpr auto goalPotentialChanceDiscountFactorEven = 200;
        constexpr auto goalVirtualChanceDiscountFactorBehind = 200;
        constexpr auto goalVirtualChanceDiscountFactorInLead = 0;
        constexpr auto goalVirtualChanceDiscountFactorEven = 100;
        constexpr auto baseVal = 300;
        constexpr auto knockoutPenalty = 500;

        double val = 0;
        int scoreDiff = 0;

        if (chaser->isFined || chaser->knockedOut) {
            return val;
        } else {
            val += baseVal;
        }
        if(chaser->knockedOut){
            val -= knockoutPenalty;
        }

        if(env->team1->hasMember(chaser)){
            scoreDiff = env->team1->score - env->team2->score;
        } else {
            scoreDiff = env->team2->score - env->team1->score;
        }

        //If Chaser holds quaffle
        if (chaser->position == env->quaffle->position) {
            val += holdsQuaffleBaseDiscount;
            if(scoreDiff < -gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, chaser) * goalChanceDiscountFactorBehind;
            } else if(scoreDiff > gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, chaser) * goalChanceDiscountFactorInLead;
            } else {
                val += getHighestGoalRate(env, chaser) * goalChanceDiscountFactorEven;
            }
        } else {
            if(teamHasQuaffle(env, chaser)) {
                if(scoreDiff < -gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorBehind;
                } else if(scoreDiff > gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorInLead;
                } else {
                    val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorEven;
                }
            } else {
                val += baseQuaffleDistanceDiscount / gameController::getDistance(chaser->position, env->quaffle->position);
                if(scoreDiff < -gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, chaser) * goalVirtualChanceDiscountFactorBehind;
                } else if(scoreDiff > gameController::SNITCH_POINTS) {
                    val += getHighestGoalRate(env, chaser) * goalVirtualChanceDiscountFactorInLead;
                } else {
                    val += getHighestGoalRate(env, chaser) * goalVirtualChanceDiscountFactorEven;
                }
            }
        }
        return val;
    }

    double evalBludgers(const std::shared_ptr<const gameModel::Environment> &env, gameModel::TeamSide mySide) {
        constexpr auto keeperBaseThreat = 500.0;
        constexpr auto seekerBaseThreat = 550.0;
        constexpr auto chaserBaseThreat = 500.0;
        constexpr auto beaterBaseThreat = 400.0;
        constexpr auto beaterHoldsBludgerDiscount = 500;

        auto calcThreat = [&env, &mySide](const std::shared_ptr<const gameModel::Team> &team){
            double val = 0;
            for(const auto &bludger : env->bludgers){
                const auto &bPos = bludger->position;
                //eval team1
                if (!team->keeper->isFined && gameController::getDistance(team->keeper->position, bPos) == 1) {
                    val -= keeperBaseThreat;
                }

                if (!team->seeker->isFined && gameController::getDistance(team->seeker->position, bPos) == 1 && env->snitch->exists) {
                    val -= seekerBaseThreat;
                }

                for (const auto &chaser : team->chasers) {
                    if (!chaser->isFined && gameController::getDistance(bPos, chaser->position) == 1) {
                        val -= chaserBaseThreat;
                    }
                }


            }

            for (const auto &beater : team->beaters) {
                auto bPos = env->bludgers[0]->position;

                if(gameController::getDistance(beater->position, env->bludgers[1]->position) < gameController::getDistance(beater->position, bPos)){
                    bPos = env->bludgers[1]->position;
                }
                if (beater->position != bPos) {
                    val += beaterBaseThreat / gameController::getDistance(bPos, beater->position);
                } else {
                    val += beaterHoldsBludgerDiscount;
                }
            }

            return team->getSide() == mySide ? val : -val;
        };


        return calcThreat(env->team1) + calcThreat(env->team2);
    }

    double getHighestGoalRate(const std::shared_ptr<gameModel::Environment> &env,
            const std::shared_ptr<gameModel::Player> &actor) {
        double chance = 0;
        auto goalPos = gameModel::Environment::getGoalsRight();

        if (env->getTeam(actor)->getSide() == gameModel::TeamSide::LEFT) {
            goalPos = env->getGoalsLeft();
        }

        for (const auto &goal : goalPos) {
            gameController::Shot shotSim(env, actor, env->quaffle, goal);

            if(shotSim.isShotOnGoal() == gameController::ActionResult::ScoreLeft
                    || shotSim.isShotOnGoal() == gameController::ActionResult::ScoreRight) {

                if (shotSim.successProb() > chance) {
                    chance = shotSim.successProb();
                }
            }

        }

        return chance;
    }


    bool teamHasQuaffle(const std::shared_ptr<const gameModel::Environment> &env, const std::shared_ptr<const gameModel::Player> &player) {
        auto playerOnQuaffle = env->getPlayer(env->quaffle->position);
        if(playerOnQuaffle.has_value()){
            return env->getTeam(player)->hasMember(*playerOnQuaffle);
        }

        return false;
    }

    double hypotheticalShotSuccessProb(const std::shared_ptr<gameModel::Environment> &env, gameModel::TeamSide teamSide){
        using namespace communication::messages::types;
        auto id = EntityId::LEFT_CHASER1;
        auto goals = gameModel::Environment::getGoalsRight();
        if(teamSide == gameModel::TeamSide::RIGHT){
            id = EntityId::RIGHT_CHASER1;
            goals = gameModel::Environment::getGoalsLeft();
        }

        auto nonExistingPlayer = std::make_shared<gameModel::Chaser>(env->quaffle->position, Broom::FIREBOLT, id);
        nonExistingPlayer->knockedOut = false;
        nonExistingPlayer->isFined = false;

        double highestChance = 0;
        for(const auto &goal : goals){
            gameController::Shot prototype(env, nonExistingPlayer, env->quaffle, goal);
            auto success = prototype.successProb();
            if(success > highestChance){
                highestChance = success;
            }
        }

        return highestChance;
    }

    double simpleEval(const aiTools::State &state, gameModel::TeamSide mySide) {
        constexpr auto halfGoal = gameController::GOAL_POINTS / 2;
        constexpr auto disqPenalty = 1000;
        constexpr auto seekerBanPenalty = 1000000;
        double val = 0;
        auto otherSide = mySide == gameModel::TeamSide::LEFT ? gameModel::TeamSide::RIGHT : gameModel::TeamSide::LEFT;
        //Score difference
        auto scoreDiff = state.env->getTeam(mySide)->score - state.env->getTeam(otherSide)->score;
        val += scoreDiff;

        //Eval quaffle players
        if(teamHasQuaffle(state.env, state.env->getTeam(mySide)->seeker)){
            //Holding quaffle counts as half a goal
            val += halfGoal;
        } else if(teamHasQuaffle(state.env, state.env->getTeam(otherSide)->seeker)) {
            //Holding quaffle counts as half a goal
            val -= halfGoal;
        } else {
            //Calc distance advantage over opponent
            std::vector<int> myPlayerDistances;
            std::vector<int> opPlayerDistances;
            myPlayerDistances.reserve(4);
            opPlayerDistances.reserve(4);
            for(const auto &player : state.env->getAllPlayers()) {
                bool canHoldQuaffle = INSTANCE_OF(player, gameModel::Keeper) || INSTANCE_OF(player, gameModel::Chaser);
                if(player->isFined || player->knockedOut || !canHoldQuaffle){
                    continue;
                }

                auto dist = gameController::getDistance(player->position, state.env->quaffle->position);
                if(gameLogic::conversions::idToSide(player->getId()) == mySide) {
                    myPlayerDistances.emplace_back(dist);
                } else if(gameLogic::conversions::idToSide(player->getId()) == otherSide) {
                    opPlayerDistances.emplace_back(dist);
                }
            }

            std::sort(myPlayerDistances.begin(), myPlayerDistances.end());
            std::sort(opPlayerDistances.begin(), opPlayerDistances.end());
            if(myPlayerDistances.size() != opPlayerDistances.size()){
                auto &smaller = myPlayerDistances.size() < opPlayerDistances.size() ? myPlayerDistances : opPlayerDistances;
                auto &larger =  myPlayerDistances.size() < opPlayerDistances.size() ? opPlayerDistances : myPlayerDistances;
                for(std::size_t i = 0; i < smaller.size() - larger.size(); i++){
                    smaller.emplace_back(std::numeric_limits<int>::max());
                }
            }

            double decay = 0.5;
            for(std::size_t i = 0; i < myPlayerDistances.size(); i++){
                val += std::max(std::min(opPlayerDistances[i] - myPlayerDistances[i], gameController::GOAL_POINTS), -gameController::GOAL_POINTS) * decay;
                decay /= 2;
            }

        }

        //Eval seeker
        if(state.env->snitch->exists){
            auto mySeeker = state.env->getTeam(mySide)->seeker;
            auto opponentSeeker = state.env->getTeam(otherSide)->seeker;
            bool mySeekerIncapacitated = mySeeker->isFined || mySeeker->knockedOut;
            bool opSeekerIncapacitated = opponentSeeker->isFined || opponentSeeker->knockedOut;
            if(!mySeekerIncapacitated && mySeeker->position == state.env->snitch->position){
                if(scoreDiff < -gameController::SNITCH_POINTS) {
                    val -= gameController::SNITCH_POINTS;
                } else {
                    val += gameController::SNITCH_POINTS;
                }
            } else if(!opSeekerIncapacitated && opponentSeeker->position == state.env->snitch->position){
                if(scoreDiff > gameController::SNITCH_POINTS) {
                    val += gameController::SNITCH_POINTS;
                } else {
                    val -= gameController::SNITCH_POINTS;
                }
            }

            auto myDistance = gameController::getDistance(mySeeker->position, state.env->snitch->position);
            auto opDistance = gameController::getDistance(opponentSeeker->position, state.env->snitch->position);
            auto distDiff = 8 * (opDistance - myDistance);

            if(!mySeeker->isFined && !opponentSeeker->isFined) {
                val += distDiff;
            }

            if (mySeeker->isFined&& !opponentSeeker->isFined) {
                val -= seekerBanPenalty;
            }

            if(opponentSeeker->isFined && !mySeeker->isFined) {
                val += 16 - myDistance;
            }
        }

        //Meta data
        int myKnockoutCount = 0;
        int opKnockoutCount = 0;
        for(const auto &player : state.env->getAllPlayers()){
            if(player->knockedOut){
                if(gameLogic::conversions::idToSide(player->getId()) == mySide) {
                    myKnockoutCount++;
                } else {
                    opKnockoutCount++;
                }
            }
        }

        // Knockout advantage
        auto knockOutDiff = opKnockoutCount - myKnockoutCount;
        val += 2 * knockOutDiff;

        // Ban advantage
        auto banDiff = state.env->getTeam(otherSide)->numberOfBannedMembers()
                - state.env->getTeam(mySide)->numberOfBannedMembers();
        val += 2 * banDiff;

        //Disqualification penalty
        if(state.env->getTeam(mySide)->numberOfBannedMembers() > 2 && !state.goalScoredThisRound){
            val -= disqPenalty;
        }

        //Goal chance advantage
        auto chanceDiff = hypotheticalShotSuccessProb(state.env, mySide)
                - hypotheticalShotSuccessProb(state.env, otherSide);
        val += halfGoal * chanceDiff;

        return val;
    }

}

