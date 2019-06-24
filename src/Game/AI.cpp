//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include "AITools.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/conversions.h>

#define distanceSnitchToSeeker 2

namespace ai{
    constexpr auto minShotSuccessProb = 0.2;

    double evalState(const std::shared_ptr<const gameModel::Environment> &env, gameModel::TeamSide mySide, bool goalScoredThisRound) {
        constexpr auto disqPenalty = 2000;
        constexpr auto unbanDiscountFactor = 150;
        constexpr auto maxBanCount = 3;
        constexpr auto farBehindPenalty = 700;
        constexpr auto scoreDiffFarBehindDiscountFactor = 10;
        constexpr auto scoreDiffDiscountFactor = 15;

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
            val -= farBehindPenalty + scoreDiff * scoreDiffFarBehindDiscountFactor;
        } else if(scoreDiff > gameController::SNITCH_POINTS) {
            val += farBehindPenalty + scoreDiff * scoreDiffFarBehindDiscountFactor;
        } else {
            val += scoreDiff * scoreDiffDiscountFactor;
        }

        //If the KI does not play left, return negative val
        return mySide == gameModel::TeamSide::LEFT ? val : -val;
    }

    double evalTeam(const std::shared_ptr<const gameModel::Team> &team,
                    const std::shared_ptr<gameModel::Environment> &env) {

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
                if(gameController::getDistance(seeker->position, env->snitch->position) > 6) {
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

    double evalKeeper(const std::shared_ptr<gameModel::Keeper> &keeper,
                      const std::shared_ptr<gameModel::Environment> &env) {
        constexpr auto holdsQuaffleBaseDiscount = 100;
        constexpr auto keeperBonusEvenWinChance = 20;
        constexpr auto keeperBonusHighWinChance = 500;
        constexpr auto goalChanceDiscountFactorBehind = 200;
        constexpr auto goalChanceDiscountFactorInLead = 20;
        constexpr auto goalChanceDiscountFactorEven = 50;
        constexpr auto baseQuaffleDistanceDiscount = 100.0;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 100;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 10;
        constexpr auto goalPotentialChanceDiscountFactorEven = 20;
        constexpr auto baseVal = 150;

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
            val += baseQuaffleDistanceDiscount / gameController::getDistance(keeper->position, env->quaffle->position);
            if(scoreDiff < -gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorBehind;
            } else if(scoreDiff > gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorInLead;
            } else {
                val += getHighestGoalRate(env, keeper) * goalPotentialChanceDiscountFactorEven;
            }
        }

        return val;
    }

    double evalChaser(const std::shared_ptr<gameModel::Chaser> &chaser,
                      const std::shared_ptr<gameModel::Environment> &env) {
        constexpr auto goalChanceDiscountFactorBehind = 200;
        constexpr auto goalChanceDiscountFactorInLead = 40;
        constexpr auto goalChanceDiscountFactorEven = 100;
        constexpr auto baseQuaffleDistanceDiscount = 100.0;
        constexpr auto holdsQuaffleBaseDiscount = 100;
        constexpr auto goalPotentialChanceDiscountFactorBehind = 100;
        constexpr auto goalPotentialChanceDiscountFactorInLead = 20;
        constexpr auto goalPotentialChanceDiscountFactorEven = 50;
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
            val += baseQuaffleDistanceDiscount / gameController::getDistance(chaser->position, env->quaffle->position);
            if(scoreDiff < -gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorBehind;
            } else if(scoreDiff > gameController::SNITCH_POINTS) {
                val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorInLead;
            } else {
                val += getHighestGoalRate(env, chaser) * goalPotentialChanceDiscountFactorEven;
            }
        }
        return val;
    }

    double evalBludgers(const std::shared_ptr<const gameModel::Environment> &env, gameModel::TeamSide mySide) {
        constexpr auto keeperBaseThreat = 10.0;
        constexpr auto seekerBaseThreat = 20.0;
        constexpr auto chaserBaseThreat = 10.0;
        constexpr auto beaterBaseThreat = 20.0;

        auto calcThreat = [&env, &mySide](const std::shared_ptr<const gameModel::Team> &team){
            double val = 0;
            for(const auto &bludger : env->bludgers){
                const auto &bPos = bludger->position;
                //eval team1
                if (team->keeper->position != bludger->position) {
                    val -= keeperBaseThreat / gameController::getDistance(bPos, env->team1->keeper->position);
                }

                if (team->seeker->position != bPos) {
                    val -= seekerBaseThreat / gameController::getDistance(bPos, env->team1->seeker->position);
                }

                for (const auto &chaser : team->chasers) {
                    if (chaser->position != bPos) {
                        val -= chaserBaseThreat / gameController::getDistance(bPos, chaser->position);
                    }
                }

                for (const auto &beater : team->beaters) {
                    if (beater->position != bPos) {
                        val += beaterBaseThreat / gameController::getDistance(bPos, beater->position);
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
            if (shotSim.successProb() > chance) {
                chance = shotSim.successProb();
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

    auto computeBestMove(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                    bool goalScoredThisRound) -> communication::messages::request::DeltaRequest {
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
        if(score < evalState(env, mySide, goalScoredThisRound)) {
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }

        return request::DeltaRequest{types::DeltaType::MOVE, std::nullopt, std::nullopt, std::nullopt, best->getTarget().x,
                                     best->getTarget().y, id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    }

    auto computeBestShot(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                    bool goalScoredThisRound) -> communication::messages::request::DeltaRequest {
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
        if(score < evalState(env, mySide, goalScoredThisRound)){
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

    auto computeBestWrest(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
                     bool goalScoredThisRound) -> communication::messages::request::DeltaRequest {
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

        if(currentScore < wrestScore){
            return request::DeltaRequest{types::DeltaType::WREST_QUAFFLE, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                         id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        } else {
            return request::DeltaRequest{types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, id,
                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }
    }

    auto getDistance(const gameModel::Position &startPoint, const gameModel::Position &endPoint) -> int {

        // check if cells are valid
        if (gameModel::Environment::getCell(startPoint) == gameModel::Cell::OutOfBounds ||
            gameModel::Environment::getCell(endPoint) == gameModel::Cell::OutOfBounds){

            return -1;
        }

        // check if start and end point are equal
        if (startPoint == endPoint){
            return 0;
        }

        int totalDistance = 0;

        // calc the differences within the components of the given points
        int dX = std::abs(startPoint.x - endPoint.x);
        int dY = std::abs(startPoint.y - endPoint.y);

        // calculate the total distance
        if (dX >= dY) {
            totalDistance += dY;
            totalDistance += dX-dY;
        } else {
            totalDistance += dX;
            totalDistance += dY-dX;
        }

        return totalDistance;
    }

    auto getNextFanTurn(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
                        communication::messages::broadcast::Next &next, const gameController::ExcessLength &excessLength) -> const communication::messages::request::DeltaRequest {
        communication::messages::types::EntityId activeEntityId = next.getEntityId();
        std::optional<communication::messages::types::EntityId> passiveEntityId;
        if (activeEntityId == communication::messages::types::EntityId::LEFT_NIFFLER ||
            activeEntityId == communication::messages::types::EntityId::RIGHT_NIFFLER) {
            if(isNifflerUseful(mySide, env)){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SNITCH_SNATCH, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_ELF ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_ELF){
            passiveEntityId = isElfUseful(mySide, env, excessLength);
            if(passiveEntityId.has_value()){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::ELF_TELEPORTATION, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_TROLL ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_TROLL){
            if(isTrollUseful(mySide, env)){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::TROLL_ROAR, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_GOBLIN ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_GOBLIN){
            passiveEntityId = getGoblinTarget(mySide, env);
            if(passiveEntityId.has_value()){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::GOBLIN_SHOCK, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else{
            return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                  std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }
    }

    bool isNifflerUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env){
        if (mySide == env->team1->side) {
            return getDistance(env->team2->seeker->position, env->snitch->position) <= 2;
        }else{
            return getDistance(env->team1->seeker->position, env->snitch->position) <= 2;
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

    auto getGoblinTarget(const gameModel::TeamSide &mySide,
                        const std::shared_ptr<const gameModel::Environment> &env) -> const std::optional<communication::messages::types::EntityId> {
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

    auto isElfUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
                     const gameController::ExcessLength &excessLength) -> const std::optional<communication::messages::types::EntityId> {
        if(env->snitch->exists) {
            std::shared_ptr<gameModel::Environment> environment = env->clone();
            gameController::moveSnitch(environment->snitch, environment, excessLength);
            if (mySide == environment->team1->side) {
                if (getDistance(environment->team2->seeker->position, environment->snitch->position) <= distanceSnitchToSeeker) {
                    return environment->team2->seeker->id;
                }
            } else {
                if (getDistance(environment->team1->seeker->position, environment->snitch->position) <= distanceSnitchToSeeker) {
                    return environment->team1->seeker->id;
                }
            }
            return std::nullopt;
        }else{
            return std::nullopt;
        }
    }
}