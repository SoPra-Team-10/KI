//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/conversions.h>
#include <SopraAITools/AITools.h>

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
        val = valTeam1 - valTeam2 + valBludgers;

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
        constexpr auto nearCenterDiscount = 15.0;
        constexpr auto baseVal = 200;

        constexpr auto centerX = 8;
        constexpr auto centerY = 6;

        double val = 0;

        if(seeker->isFined || seeker->knockedOut) {
            return val;
        } else {
            val += baseVal;
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
                    val = -gameLosePenalty * env->config.gameDynamicsProbs.catchSnitch;
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
        constexpr auto holdsQuaffleBaseDiscount = 250;
        constexpr auto keeperBonusEvenWinChance = 20;
        constexpr auto keeperBonusHighWinChance = 500;
        constexpr auto goalChanceDiscountFactorBehind = 1000;
        constexpr auto goalChanceDiscountFactorInLead = 200;
        constexpr auto goalChanceDiscountFactorEven = 400;
        constexpr auto baseQuaffleDistanceDiscount = 100.0;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 500;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 100;
        constexpr auto goalPotentialChanceDiscountFactorEven = 200;
        constexpr auto baseVal = 150;
        constexpr auto keeperBonusPotentialEvenWinChance = 10;
        constexpr auto keeperBonusPotentialHighWinChance = 200;

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
        constexpr auto goalChanceDiscountFactorEven = 400;
        constexpr auto baseQuaffleDistanceDiscount = 100.0;
        constexpr auto holdsQuaffleBaseDiscount = 250;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 500;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 100;
        constexpr auto goalPotentialChanceDiscountFactorEven = 200;
        constexpr auto goalVirtualChanceDiscountFactorBehind = 200;
        constexpr auto goalVirtualChanceDiscountFactorInLead = 0;
        constexpr auto goalVirtualChanceDiscountFactorEven = 100;
        constexpr auto baseVal = 100;

        double val = 0;
        int scoreDiff = 0;

        if (chaser->isFined || chaser->knockedOut) {
            return val;
        } else {
            val += baseVal;
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
        constexpr auto keeperBaseThreat = 200.0;
        constexpr auto seekerBaseThreat = 250.0;
        constexpr auto chaserBaseThreat = 200.0;
        constexpr auto beaterBaseThreat = 120.0;
        constexpr auto beaterHoldsBludgerDiscount = 140;

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

                for (const auto &beater : team->beaters) {
                    if (beater->position != bPos) {
                        val += beaterBaseThreat / gameController::getDistance(bPos, beater->position);
                    } else {
                        val += beaterHoldsBludgerDiscount;
                    }
                }
            }

            return team->side == mySide ? val : -val;
        };


        return calcThreat(env->team1) + calcThreat(env->team2);
    }

    double getHighestGoalRate(const std::shared_ptr<gameModel::Environment> &env,
            const std::shared_ptr<gameModel::Player> &actor) {
        double chance = 0;
        auto goalPos = env->getGoalsRight();

        if (env->getTeam(actor)->side == gameModel::TeamSide::LEFT) {
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

}