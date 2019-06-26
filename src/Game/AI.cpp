//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include "AITools.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/conversions.h>

namespace ai{
    constexpr auto minShotSuccessProb = 0.2;
    constexpr int distanceSnitchSeeker = 2;

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
        constexpr auto nearCenterDiscount = 30.0;
        constexpr auto baseVal = 200;

        constexpr auto centerX = 8;
        constexpr auto centerY = 6;

        double val = 0;

        if(seeker->isFined) {
            return val;
        } else {
            val += baseVal;
        }
        if(seeker->knockedOut) {
            return val;
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
                    val = winSnitchDistanceDiscount / (computeOptimalPath(seeker, env->snitch->position, env).size() + 1);
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

        if (keeper->isFined) {
            return val;
        } else{
            val += baseVal;
        }
        if(keeper->knockedOut) {
            return val;
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

        if (chaser->isFined) {
            return val;
        } else {
            val += baseVal;
        }
        if(chaser->knockedOut) {
            return val;
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
        constexpr auto keeperBaseThreat = 15.0;
        constexpr auto seekerBaseThreat = 25.0;
        constexpr auto chaserBaseThreat = 15.0;
        constexpr auto beaterBaseThreat = 30.0;
        constexpr auto beaterHoldsBludgerDiscount = 40;

        auto calcThreat = [&env, &mySide](const std::shared_ptr<const gameModel::Team> &team){
            double val = 0;
            for(const auto &bludger : env->bludgers){
                const auto &bPos = bludger->position;
                //eval team1
                if (!team->keeper->isFined && team->keeper->position != bludger->position) {
                    val -= keeperBaseThreat / gameController::getDistance(bPos, env->team1->keeper->position);
                }

                if (!team->seeker->isFined && team->seeker->position != bPos) {
                    val -= seekerBaseThreat / gameController::getDistance(bPos, env->team1->seeker->position);
                }

                for (const auto &chaser : team->chasers) {
                    if (!chaser->isFined && chaser->position != bPos) {
                        val -= chaserBaseThreat / gameController::getDistance(bPos, chaser->position);
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


    auto computeOptimalPath(const std::shared_ptr<const gameModel::Player> &player, const gameModel::Position &destination,
                            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position> {
        if(player->position == destination) {
            return {};
        }

        auto expandFunction = [&env, &player](const aiTools::SearchNode<gameModel::Position> &node) {
            std::vector<aiTools::SearchNode<gameModel::Position>> path;
            path.reserve(8);
            auto parent = std::make_shared<aiTools::SearchNode<gameModel::Position>>(node);
            for (const auto &pos : env->getAllLegalCellsAround(node.state, env->team1->hasMember(player))) {
                path.emplace_back(pos, parent, node.pathCost + 1);
            }

            return path;
        };

        auto evalFunction = [&destination](const aiTools::SearchNode<gameModel::Position> &pos) {
            int g = pos.parent.has_value() ? pos.parent.value()->pathCost + 1 : 1;
            return g + gameController::getDistance(pos.state, destination);
        };

        aiTools::SearchNode<gameModel::Position> startNode(player->position, std::nullopt, 0);
        aiTools::SearchNode<gameModel::Position> destinationNode(destination, std::nullopt, 0);
        auto res = aiTools::aStarSearch(startNode, destinationNode, expandFunction, evalFunction);
        std::vector<gameModel::Position> ret;
        if (res.has_value()) {
            ret.reserve(res->pathCost + 1);
            ret.emplace_back(res->state);
            auto parent = res->parent;
            while (parent.has_value()) {
                ret.emplace_back((*parent)->state);
                parent = (*parent)->parent;
            }
        }

        return ret;
    }

    bool teamHasQuaffle(const std::shared_ptr<const gameModel::Environment> &env, const std::shared_ptr<const gameModel::Player> &player) {
        auto playerOnQuaffle = env->getPlayer(env->quaffle->position);
        if(playerOnQuaffle.has_value()){
            return env->getTeam(player)->hasMember(*playerOnQuaffle);
        }

        return false;
    }

    auto computeBestMove(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                    bool goalScoredThisRound, util::Logging &log) -> communication::messages::request::DeltaRequest {
        using namespace communication::messages;
        auto mySide = gameLogic::conversions::idToSide(id);
        auto player = env->getPlayerById(id);
        auto moves = gameController::getAllPossibleMoves(player, env);
        if(moves.empty()){
            throw std::runtime_error("No move possible");
        }

        auto evalFun = [mySide, goalScoredThisRound] (const std::shared_ptr<const gameModel::Environment> &e){
            return evalState(e, mySide, goalScoredThisRound);
        };

        auto [best, score] = aiTools::chooseBestAction(moves, evalFun);
        auto currScore = evalState(env, mySide, goalScoredThisRound);
        log.debug(std::string("Current env value: ") + std::to_string(currScore));
        log.debug(std::string("Best move env value: ") + std::to_string(score));
        if(score < currScore) {
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }

        return request::DeltaRequest{types::DeltaType::MOVE, std::nullopt, std::nullopt, std::nullopt, best->getTarget().x,
                                     best->getTarget().y, id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    }

    auto computeBestShot(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                    bool goalScoredThisRound, util::Logging &log) -> communication::messages::request::DeltaRequest {
        using namespace communication::messages;
        auto mySide = gameLogic::conversions::idToSide(id);
        auto player = env->getPlayerById(id);
        auto shots = gameController::getAllPossibleShots(player, env, minShotSuccessProb);
        if(shots.empty()){
            //No shot with decent success probability possible -> skip turn
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }

        auto evalFun = [mySide, goalScoredThisRound] (const std::shared_ptr<const gameModel::Environment> &e){
            return evalState(e, mySide, goalScoredThisRound);
        };

        auto [best, score] = aiTools::chooseBestAction(shots, evalFun);
        auto currScore = evalState(env, mySide, goalScoredThisRound);
        log.debug(std::string("Current env value: ") + std::to_string(currScore));
        log.debug(std::string("Best shot env value: ") + std::to_string(score));
        if(score < currScore){
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }

        if(INSTANCE_OF(best->getBall(), const gameModel::Quaffle)){
            return request::DeltaRequest{types::DeltaType::QUAFFLE_THROW, std::nullopt, std::nullopt, std::nullopt, best->getTarget().x,
                                         best->getTarget().y, id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        } else if(INSTANCE_OF(best->getBall(), const gameModel::Bludger)){
            return request::DeltaRequest{types::DeltaType::BLUDGER_BEATING, std::nullopt, std::nullopt, std::nullopt, best->getTarget().x,
                                         best->getTarget().y, id, best->getBall()->id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        } else {
            throw std::runtime_error("Invalid shot was calculated");
        }

    }

    auto
    computeBestWrest(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                     bool goalScoredThisRound, util::Logging &log) -> communication::messages::request::DeltaRequest {
        using namespace communication::messages;
        auto mySide = gameLogic::conversions::idToSide(id);
        auto player = std::dynamic_pointer_cast<gameModel::Chaser>(env->getPlayerById(id));
        if(!player){
            throw std::runtime_error("Player is no Chaser");
        }

        gameController::WrestQuaffle wrest(env, player, env->quaffle->position);
        if(wrest.check() == gameController::ActionCheckResult::Impossible){
            throw std::runtime_error("Wrest is impossible");
        }

        auto currentScore = evalState(env, mySide, goalScoredThisRound);
        double wrestScore = 0;
        for(const auto &outcome : wrest.executeAll()){
            wrestScore += outcome.second * evalState(outcome.first, mySide, goalScoredThisRound);
        }

        log.debug(std::string("Current env value: ") + std::to_string(currentScore));
        log.debug(std::string("Best wrest env value: ") + std::to_string(wrestScore));
        if(currentScore < wrestScore){
            return request::DeltaRequest{types::DeltaType::WREST_QUAFFLE, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                         id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        } else {
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }
    }

    auto redeployPlayer(const std::shared_ptr<const gameModel::Environment> &env,
                        communication::messages::types::EntityId id,
                        util::Logging &log) -> communication::messages::request::DeltaRequest {
        using namespace communication::messages;
        auto mySide = gameLogic::conversions::idToSide(id);
        auto bestScore = -std::numeric_limits<double>::infinity();
        gameModel::Position redeployPos(0, 0);
        for(const auto &pos : env->getFreeCellsForRedeploy(mySide)){
            auto newEnv = env->clone();
            newEnv->getPlayerById(id)->position = pos;
            newEnv->getPlayerById(id)->isFined = false;
            auto score = evalState(newEnv, mySide, false);
            if(score > bestScore) {
                bestScore = score;
                redeployPos = pos;
            }
        }

        log.debug("Redeploy");
        return request::DeltaRequest{types::DeltaType::UNBAN, std::nullopt, std::nullopt, std::nullopt, redeployPos.x, redeployPos.y, id,
                                     std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    }

    auto getNextFanTurn(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
                        const communication::messages::broadcast::Next &next, const gameController::ExcessLength &excessLength) ->
                        const communication::messages::request::DeltaRequest {
        using namespace communication::messages;
        auto activeEntityId = next.getEntityId();
        std::optional<types::EntityId> passiveEntityId;
        if (activeEntityId == types::EntityId::LEFT_NIFFLER ||
            activeEntityId == types::EntityId::RIGHT_NIFFLER) {
            if(isNifflerUseful(mySide, env)){
                return request::DeltaRequest{types::DeltaType::SNITCH_SNATCH, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == types::EntityId::LEFT_ELF ||
                 activeEntityId == types::EntityId::RIGHT_ELF){
            passiveEntityId = getElfTarget(mySide, env, excessLength);
            if(passiveEntityId.has_value()){
                return request::DeltaRequest{types::DeltaType::ELF_TELEPORTATION, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == types::EntityId::LEFT_TROLL ||
                 activeEntityId == types::EntityId::RIGHT_TROLL){
            if(isTrollUseful(mySide, env)){
                return request::DeltaRequest{types::DeltaType::TROLL_ROAR, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == types::EntityId::LEFT_GOBLIN ||
                 activeEntityId == types::EntityId::RIGHT_GOBLIN){
            passiveEntityId = getGoblinTarget(mySide, env);
            if(passiveEntityId.has_value()){
                return request::DeltaRequest{types::DeltaType::GOBLIN_SHOCK, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                             std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else{
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                         std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }
    }

    bool isNifflerUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env){
        if (mySide == env->team1->side) {
            return gameController::getDistance(env->team2->seeker->position, env->snitch->position) <= distanceSnitchSeeker;
        }else{
            return gameController::getDistance(env->team1->seeker->position, env->snitch->position) <= distanceSnitchSeeker;
        }
    }

    bool isTrollUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env) {
        auto player = env->getPlayer(env->quaffle->position);
        if(player.has_value()) {
            if(mySide != env->getTeam(player.value())->side) {
                return true;
            }
        }
        return false;
    }

    auto getGoblinTarget(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env)->
        const std::optional<communication::messages::types::EntityId> {
        auto opponentGoals = env->getGoalsRight();
        if(mySide == gameModel::TeamSide::RIGHT) {
            opponentGoals = env->getGoalsLeft();
        }

        for(const auto &goal : opponentGoals){
            auto player = env->getPlayer(goal);
            if(player.has_value() && !env->getTeam(mySide)->hasMember(player.value())){
                return player.value()->id;
            }
        }
        return std::nullopt;
    }

    auto getElfTarget(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
                      const gameController::ExcessLength &excessLength) -> const std::optional<communication::messages::types::EntityId> {
        if(env->snitch->exists) {
            std::shared_ptr<gameModel::Environment> environment = env->clone();
            gameController::moveSnitch(environment->snitch, environment, excessLength);
            if (mySide == environment->team1->side) {
                if (gameController::getDistance(environment->team2->seeker->position, environment->snitch->position) <= distanceSnitchSeeker) {
                    return environment->team2->seeker->id;
                }
            } else {
                if (gameController::getDistance(environment->team1->seeker->position, environment->snitch->position) <= distanceSnitchSeeker) {
                    return environment->team1->seeker->id;
                }
            }
        }
        return std::nullopt;
    }
}