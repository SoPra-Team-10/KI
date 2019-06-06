/**
 * @file Game.cpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#include "Game.hpp"
#include <utility>

Game::Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig) :
    difficulty(difficulty), myConfig(std::move(ownTeamConfig)){}

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
    if(matchStart.getLeftTeamUserName() == myConfig.getTeamName()){
        side = TeamSide::LEFT;
        theirConfig = matchStart.getRightTeamConfig();
    } else {
        side = TeamSide::RIGHT;
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

    auto team1 = teamFromSnapshot(snapshot.getLeftTeam(), TeamSide::LEFT);
    auto team2 = teamFromSnapshot(snapshot.getRightTeam(), TeamSide::RIGHT);

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

void Game::mirrorPos(gameModel::Position &pos) const{
    pos.x = FIELD_WIDTH - pos.x;
}



