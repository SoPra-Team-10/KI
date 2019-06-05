#include <utility>

/**
 * @file Game.cpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#include "Game.hpp"

Game::Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig) :
    difficulty(difficulty), myConfig(std::move(ownTeamConfig)){}

auto Game::getTeamFormation(const communication::messages::broadcast::MatchStart &matchStart)
    -> communication::messages::request::TeamFormation {
    currentEnv = std::make_shared<gameModel::Environment>(gameModel::Config{matchStart.getMatchConfig()}, nullptr, nullptr);
    if(matchStart.getLeftTeamUserName() == myConfig.getTeamName()){
        side = TeamSide::LEFT;
        theirConfig = matchStart.getRightTeamConfig();
        return {3, 8, 3, 6, 7, 4, 6, 6, 7, 8, 6, 5, 6, 7};
    } else {
        side = TeamSide::RIGHT;
        theirConfig = matchStart.getLeftTeamConfig();
        return {13, 8, 13, 6, 9, 4, 10, 6, 9, 8, 10, 4, 10, 7};
    }
}

void Game::onSnapshot(const communication::messages::broadcast::Snapshot &snapshot) {
    using namespace communication::messages::types;
    currentRound = snapshot.getRound();
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

    currentEnv->team1 = teamFromSnapshot(snapshot.getLeftTeam(), TeamSide::LEFT);
    currentEnv->team2 = teamFromSnapshot(snapshot.getRightTeam(), TeamSide::RIGHT);
    currentEnv->quaffle = quaf;
    currentEnv->bludgers = bludgers;
    currentEnv->snitch = snitch;
    currentEnv->pileOfShit = pileOfShit;
}

auto Game::getNextAction(const communication::messages::broadcast::Next &)
    -> communication::messages::request::DeltaRequest {
    return communication::messages::request::DeltaRequest();
}

auto Game::teamFromSnapshot(const communication::messages::broadcast::TeamSnapshot &teamSnapshot, TeamSide teamSide) const ->
        std::shared_ptr<gameModel::Team> {
    using ID = communication::messages::types::EntityId;
    auto teamConf = teamSide == side ? myConfig : theirConfig;
    bool left = teamSide == TeamSide::LEFT;
    gameModel::Seeker seeker(gameModel::Position{teamSnapshot.getSeekerX(), teamSnapshot.getSeekerY()}, "", {}, teamConf.getSeeker().getBroom(),
            left ? ID::LEFT_SEEKER : ID::RIGHT_SEEKER);
    seeker.knockedOut = teamSnapshot.isSeekerKnockout();
    seeker.isFined = teamSnapshot.isSeekerBanned();

    gameModel::Keeper keeper(gameModel::Position{teamSnapshot.getKeeperX(), teamSnapshot.getKeeperY()}, "", {}, teamConf.getKeeper().getBroom(),
            left ? ID::LEFT_KEEPER : ID::RIGHT_KEEPER);
    keeper.knockedOut = teamSnapshot.isKeeperKnockout();
    keeper.isFined = teamSnapshot.isKeeperBanned();

    std::array<gameModel::Beater, 2> beaters =
            {gameModel::Beater(gameModel::Position{teamSnapshot.getBeater1X(), teamSnapshot.getBeater1Y()}, "", {}, teamConf.getBeater1().getBroom(),
                     left ? ID::LEFT_BEATER1: ID::RIGHT_BEATER1),
             gameModel::Beater(gameModel::Position{teamSnapshot.getBeater2X(), teamSnapshot.getBeater2Y()}, "", {}, teamConf.getBeater2().getBroom(),
                     left ? ID::LEFT_BEATER2: ID::RIGHT_BEATER2)};
    beaters[0].knockedOut = teamSnapshot.isBeater1Knockout();
    beaters[0].isFined = teamSnapshot.isBeater1Banned();
    beaters[1].knockedOut = teamSnapshot.isBeater2Knockout();
    beaters[1].isFined = teamSnapshot.isBeater2Banned();

    std::array<gameModel::Chaser, 3> chasers=
            {gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser1X(), teamSnapshot.getChaser1Y()}, "", {}, teamConf.getChaser1().getBroom(),
                               left ? ID::LEFT_CHASER1 : ID::RIGHT_CHASER1),
             gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser2X(), teamSnapshot.getChaser2Y()}, "", {}, teamConf.getChaser2().getBroom(),
                               left ? ID::LEFT_CHASER2 : ID::RIGHT_CHASER2),
             gameModel::Chaser(gameModel::Position{teamSnapshot.getChaser3X(), teamSnapshot.getChaser3Y()}, "", {}, teamConf.getChaser3().getBroom(),
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
    return std::make_shared<gameModel::Team>(seeker, keeper, beaters, chasers, "", "", "", teamSnapshot.getPoints(), fans);
}


