#include <utility>

/**
 * @file Game.cpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#include "Game.hpp"
#include "AI.h"
#include <utility>
#include <SopraGameLogic/conversions.h>
#include <SopraGameLogic/GameController.h>

constexpr unsigned int OVERTIME_INTERVAL = 3;
constexpr unsigned int TIMEOUT_TOLERANCE = 2000;

Game::Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig) :
    difficulty(difficulty), myConfig(std::move(ownTeamConfig)){
    currentState.availableFansRight = {};
    currentState.availableFansLeft = {};
    currentState.playersUsedRight = {};
    currentState.playersUsedLeft = {};
}

auto Game::getTeamFormation(const communication::messages::broadcast::MatchStart &matchStart)
    -> communication::messages::request::TeamFormation {
    matchConfig = matchStart.getMatchConfig();
    if(matchStart.getLeftTeamConfig().getTeamName() == myConfig.getTeamName()){
        mySide = gameModel::TeamSide::LEFT;
        theirConfig = matchStart.getRightTeamConfig();
        return aiTools::getTeamFormation(gameModel::TeamSide::LEFT);
    } else {
        mySide = gameModel::TeamSide::RIGHT;
        theirConfig = matchStart.getLeftTeamConfig();
        return aiTools::getTeamFormation(gameModel::TeamSide::RIGHT);
    }
}

void Game::onSnapshot(const communication::messages::broadcast::Snapshot &snapshot) {
    using namespace communication::messages::types;
    currentState.roundNumber = snapshot.getRound();
    currentState.currentPhase = snapshot.getPhase();
    currentState.goalScoredThisRound = snapshot.isGoalWasThrownThisRound();
    auto lastDelta = snapshot.getLastDeltaBroadcast();
    if(lastDelta.getDeltaType() == DeltaType::TURN_USED){
        if(!lastDelta.getActiveEntity().has_value()){
            throw std::runtime_error("Active entity id not set!");
        }

        if(gameLogic::conversions::idToSide(*lastDelta.getActiveEntity()) == gameModel::TeamSide::LEFT) {
            currentState.playersUsedLeft.emplace(*lastDelta.getActiveEntity());
        } else {
            currentState.playersUsedRight.emplace(*lastDelta.getActiveEntity());
        }
    } else if(lastDelta.getDeltaType() == DeltaType::ROUND_CHANGE) {
        currentState.playersUsedRight.clear();
        currentState.playersUsedLeft.clear();
    }

    auto quaf = std::make_shared<gameModel::Quaffle>(gameModel::Position{snapshot.getQuaffleX(), snapshot.getQuaffleY()});
    auto bludgers = std::array<std::shared_ptr<gameModel::Bludger>, 2>
            {std::make_shared<gameModel::Bludger>(gameModel::Position{snapshot.getBludger1X(),
                                                                      snapshot.getBludger1Y()}, EntityId::BLUDGER1),
             std::make_shared<gameModel::Bludger>(gameModel::Position{snapshot.getBludger1X(),
                                                                      snapshot.getBludger1Y()}, EntityId::BLUDGER2)};
    gameModel::Position snitchPos = {0, 0};
    if(snapshot.getSnitchX().has_value()) {
        snitchPos = {*snapshot.getSnitchX(), *snapshot.getSnitchY()};
    }

    auto snitch = std::make_shared<gameModel::Snitch>(snitchPos);
    snitch->exists = snapshot.getSnitchX().has_value();
    std::deque<std::shared_ptr<gameModel::CubeOfShit>> pileOfShit;
    for(const auto &pieceOfShit : snapshot.getWombatCubes()) {
        pileOfShit.emplace_back(std::make_shared<gameModel::CubeOfShit>(gameModel::Position{pieceOfShit.first, pieceOfShit.second}));
    }

    auto team1 = teamFromSnapshot(snapshot.getLeftTeam(), gameModel::TeamSide::LEFT);
    auto team2 = teamFromSnapshot(snapshot.getRightTeam(), gameModel::TeamSide::RIGHT);

    if(!gotFirstSnapshot){
        gotFirstSnapshot = true;
        currentState.env = std::make_shared<gameModel::Environment>(gameModel::Config{matchConfig}, team1, team2);
        currentState.overTimeCounter = 0;
    } else {
        currentState.env->team1 = team1;
        currentState.env->team2 = team2;
        currentState.env->quaffle = quaf;
        currentState.env->bludgers = bludgers;
        currentState.env->snitch = snitch;
        currentState.env->pileOfShit = pileOfShit;
    }

    switch (currentState.overtimeState) {
        case gameController::ExcessLength::None:
            if (currentState.roundNumber == currentState.env->config.getMaxRounds()) {
                currentState.overtimeState = gameController::ExcessLength::Stage1;
            }

            break;
        case gameController::ExcessLength::Stage1:
            if (++currentState.overTimeCounter > OVERTIME_INTERVAL) {
                currentState.overtimeState = gameController::ExcessLength::Stage2;
                currentState.overTimeCounter = 0;
            }
            break;
        case gameController::ExcessLength::Stage2:
            if (currentState.env->snitch->position == gameModel::Position{8, 6} &&
               ++currentState.overTimeCounter > OVERTIME_INTERVAL) {
                currentState.overtimeState = gameController::ExcessLength::Stage3;
            }
            break;
        case gameController::ExcessLength::Stage3:
            break;

    }
}

auto Game::getNextAction(const communication::messages::broadcast::Next &next, util::Timer &timer)
    -> std::optional<communication::messages::request::DeltaRequest> {
    using namespace communication::messages;
    using namespace gameLogic::conversions;
    if(!gotFirstSnapshot){
        throw std::runtime_error("Local environment not set!");
    }

    if(isBall(next.getEntityId()) || idToSide(next.getEntityId()) != mySide){
        return std::nullopt;
    }


    bool abort = false;
    timer.setTimeout([&abort](){ abort = true; }, next.getTimout() - TIMEOUT_TOLERANCE);
    auto evalFunction = [this](const aiTools::State &state){
        return ai::evalState(state.env, mySide, state.goalScoredThisRound);
    };

    switch (next.getTurnType()){
        case communication::messages::types::TurnType::MOVE:{
            auto res = aiTools::computeBestMove(currentState, evalFunction, next.getEntityId(), abort);
            timer.stop();
            return res;
        }
        case communication::messages::types::TurnType::ACTION:{
            auto type = gameController::getPossibleBallActionType(currentState.env->getPlayerById(next.getEntityId()), currentState.env);
            if(!type.has_value()){
                throw std::runtime_error("No action possible");
            }

            if(*type == gameController::ActionType::Throw) {
                auto res = aiTools::computeBestShot(currentState, evalFunction, next.getEntityId(), abort);
                timer.stop();
                return res;
            } else if(*type == gameController::ActionType::Wrest) {
                auto res = aiTools::computeBestWrest(currentState, evalFunction, next.getEntityId());
                timer.stop();
                return res;
            } else {
                throw std::runtime_error("Unexpected action type");
            }
        }
        case communication::messages::types::TurnType::FAN:{
            auto res = aiTools::getNextFanTurn(currentState, next);
            timer.stop();
            return res;
        }
        case communication::messages::types::TurnType::REMOVE_BAN:{
            auto res = aiTools::redeployPlayer(currentState, evalFunction, next.getEntityId(), abort);
            timer.stop();
            return res;
        }
        default:
            throw std::runtime_error("Enum out of bounds");
    }
}

auto Game::teamFromSnapshot(const communication::messages::broadcast::TeamSnapshot &teamSnapshot, gameModel::TeamSide teamSide) const ->
        std::shared_ptr<gameModel::Team> {
    using ID = communication::messages::types::EntityId;
    auto teamConf = teamSide == mySide ? myConfig : theirConfig;
    bool left = teamSide == gameModel::TeamSide::LEFT;
    gameModel::Seeker seeker(gameModel::Position{teamSnapshot.getSeekerX(), teamSnapshot.getSeekerY()}, teamConf.getSeeker().getBroom(),
            left ? ID::LEFT_SEEKER : ID::RIGHT_SEEKER);
    seeker.knockedOut = teamSnapshot.isSeekerKnockout();
    seeker.isFined = teamSnapshot.isSeekerBanned();

    gameModel::Keeper keeper(gameModel::Position{teamSnapshot.getKeeperX(), teamSnapshot.getKeeperY()}, teamConf.getKeeper().getBroom(),
            left ? ID::LEFT_KEEPER : ID::RIGHT_KEEPER);
    keeper.knockedOut = teamSnapshot.isKeeperKnockout();
    keeper.isFined = teamSnapshot.isKeeperBanned();

    std::array<gameModel::Beater, 2> beaters =
            {gameModel::Beater(gameModel::Position{teamSnapshot.getBeater1X(), teamSnapshot.getBeater1Y()}, teamConf.getBeater1().getBroom(),
                     left ? ID::LEFT_BEATER1: ID::RIGHT_BEATER1),
             gameModel::Beater(gameModel::Position{teamSnapshot.getBeater2X(), teamSnapshot.getBeater2Y()}, teamConf.getBeater2().getBroom(),
                     left ? ID::LEFT_BEATER2: ID::RIGHT_BEATER2)};
    beaters[0].knockedOut = teamSnapshot.isBeater1Knockout();
    beaters[0].isFined = teamSnapshot.isBeater1Banned();
    beaters[1].knockedOut = teamSnapshot.isBeater2Knockout();
    beaters[1].isFined = teamSnapshot.isBeater2Banned();

    std::array<gameModel::Chaser, 3> chasers=
            {gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser1X(), teamSnapshot.getChaser1Y()}, teamConf.getChaser1().getBroom(),
                               left ? ID::LEFT_CHASER1 : ID::RIGHT_CHASER1),
             gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser2X(), teamSnapshot.getChaser2Y()}, teamConf.getChaser2().getBroom(),
                               left ? ID::LEFT_CHASER2 : ID::RIGHT_CHASER2),
             gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser3X(), teamSnapshot.getChaser3Y()}, teamConf.getChaser3().getBroom(),
                               left ? ID::LEFT_CHASER3 : ID::RIGHT_CHASER3)};

    chasers[0].knockedOut = teamSnapshot.isChaser1Knockout();
    chasers[0].isFined = teamSnapshot.isChaser1Banned();
    chasers[1].knockedOut = teamSnapshot.isChaser2Knockout();
    chasers[1].isFined = teamSnapshot.isChaser2Banned();
    chasers[2].knockedOut = teamSnapshot.isChaser3Knockout();
    chasers[2].isFined = teamSnapshot.isChaser3Banned();

    int teleport = 0;
    int rangedAttack = 0;
    int impulse = 0;
    int snitchPush = 0;
    int blockCell = 0;
    for(const auto &fan : teamSnapshot.getFans()) {
        if(fan.banned){
            continue;
        }

        switch(fan.fanType) {
            case communication::messages::types::FanType::GOBLIN:
                rangedAttack++;
                break;
            case communication::messages::types::FanType::TROLL:
                impulse++;
                break;
            case communication::messages::types::FanType::ELF:
                teleport++;
                break;
            case communication::messages::types::FanType::NIFFLER:
                snitchPush++;
                break;
            case communication::messages::types::FanType::WOMBAT:
                blockCell++;
                break;
        }
    }

    gameModel::Fanblock fans(teleport, rangedAttack, impulse, snitchPush, blockCell);
    return std::make_shared<gameModel::Team>(seeker, keeper, beaters, chasers, teamSnapshot.getPoints(), fans, teamSide);
}
