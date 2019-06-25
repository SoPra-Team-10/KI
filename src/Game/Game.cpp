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

Game::Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig) :
    difficulty(difficulty), myConfig(std::move(ownTeamConfig)){
    usedPlayersOpponent.reserve(7);
    usedPlayersOwn.reserve(7);
}

auto Game::getTeamFormation(const communication::messages::broadcast::MatchStart &matchStart)
    -> communication::messages::request::TeamFormation {
    using P = gameModel::Position;
    P seekerPos{3, 8};
    P keeperPos{3, 6};
    P c1Pos{7, 4};
    P c2Pos{6, 6};
    P c3Pos{7, 8};
    P b1Pos{6, 5};
    P b2Pos{6, 7};
    matchConfig = matchStart.getMatchConfig();
    if(matchStart.getLeftTeamConfig().getTeamName() == myConfig.getTeamName()){
        mySide = gameModel::TeamSide::LEFT;
        theirConfig = matchStart.getRightTeamConfig();
    } else {
        mySide = gameModel::TeamSide::RIGHT;
        theirConfig = matchStart.getLeftTeamConfig();
        mirrorPos(seekerPos);
        mirrorPos(keeperPos);
        mirrorPos(c1Pos);
        mirrorPos(c2Pos);
        mirrorPos(c3Pos);
        mirrorPos(b1Pos);
        mirrorPos(b2Pos);
    }

    return {seekerPos.x, seekerPos.y, keeperPos.x, keeperPos.y, c1Pos.x, c1Pos.y, c2Pos.x, c2Pos.y,
            c3Pos.x, c3Pos.y, b1Pos.x, b1Pos.y, b2Pos.x, b2Pos.y};
}

void Game::onSnapshot(const communication::messages::broadcast::Snapshot &snapshot) {
    using namespace communication::messages::types;
    currentRound = snapshot.getRound();
    currentPhase = snapshot.getPhase();
    goalScoredThisRound = snapshot.isGoalWasThrownThisRound();
    auto lastDelta = snapshot.getLastDeltaBroadcast();
    if(lastDelta.getDeltaType() == DeltaType::TURN_USED){
        if(!lastDelta.getActiveEntity().has_value()){
            throw std::runtime_error("Active entity id not set!");
        }

        if(gameLogic::conversions::idToSide(*lastDelta.getActiveEntity()) == mySide) {
            usedPlayersOwn.emplace_back(*lastDelta.getActiveEntity());
        } else {
            usedPlayersOpponent.emplace_back(*lastDelta.getActiveEntity());
        }
    } else if(lastDelta.getDeltaType() == DeltaType::ROUND_CHANGE) {
        usedPlayersOpponent.clear();
        usedPlayersOwn.clear();
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

    if(!currentEnv.has_value()){
        currentEnv = std::make_shared<gameModel::Environment>(gameModel::Config{matchConfig}, team1, team2);
    } else {
        currentEnv.value()->team1 = team1;
        currentEnv.value()->team2 = team2;
        currentEnv.value()->quaffle = quaf;
        currentEnv.value()->bludgers = bludgers;
        currentEnv.value()->snitch = snitch;
        currentEnv.value()->pileOfShit = pileOfShit;
    }

    switch (overTimeState){
        case gameController::ExcessLength::None:
            if(currentRound == (*currentEnv)->config.maxRounds){
                overTimeState = gameController::ExcessLength::Stage1;
            }

            break;
        case gameController::ExcessLength::Stage1:
            if(++overTimeCounter > OVERTIME_INTERVAL){
                overTimeState = gameController::ExcessLength::Stage2;
                overTimeCounter = 0;
            }
            break;
        case gameController::ExcessLength::Stage2:
            if((*currentEnv)->snitch->position == gameModel::Position{8, 6} &&
               ++overTimeCounter > OVERTIME_INTERVAL){
                overTimeState = gameController::ExcessLength::Stage3;
            }
            break;
        case gameController::ExcessLength::Stage3:
            break;
    }
}

auto Game::getNextAction(const communication::messages::broadcast::Next &next)
    -> std::optional<communication::messages::request::DeltaRequest> {
    using namespace communication::messages;
    if(!currentEnv.has_value()){
        throw std::runtime_error("Local environment not set!");
    }

    if(gameLogic::conversions::idToSide(next.getEntityId()) != mySide){
        return std::nullopt;
    }

    switch (next.getTurnType()){
        case communication::messages::types::TurnType::MOVE:
            return ai::computeBestMove(*currentEnv, next.getEntityId(), goalScoredThisRound);
        case communication::messages::types::TurnType::ACTION:{
            auto type = gameController::getPossibleBallActionType(
                    (*currentEnv)->getPlayerById(next.getEntityId()), *currentEnv);
            if(!type.has_value()){
                throw std::runtime_error("No action possible");
            }

            if(*type == gameController::ActionType::Throw) {
                return ai::computeBestShot(*currentEnv, next.getEntityId(), goalScoredThisRound);
            } else if(*type == gameController::ActionType::Wrest) {
                return ai::computeBestWrest(*currentEnv, next.getEntityId(), goalScoredThisRound);
            } else {
                throw std::runtime_error("Unexpected action type");
            }
        }
        case communication::messages::types::TurnType::FAN:
            return ai::getNextFanTurn(mySide, *currentEnv, next, overTimeState);
        case communication::messages::types::TurnType::REMOVE_BAN:
            return ai::redeployPlayer(*currentEnv, next.getEntityId());
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

void Game::mirrorPos(gameModel::Position &pos) const{
    pos.x = FIELD_WIDTH - pos.x;
}



