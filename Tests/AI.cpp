//
// Created by tim on 16.06.19.
//

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <Game/AI.h>
#include "setup.h"
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/Interference.h>


//-----------------------------------optimal path search----------------------------------------------------------------

TEST(ai_test, optimal_path){
    auto env = setup::createEnv();
    auto path = ai::computeOptimalPath(env->team2->seeker, {8, 6}, env);
    EXPECT_EQ(path.size(), 4);
    EXPECT_EQ(path.front(), gameModel::Position(8, 6));
    EXPECT_EQ(path.back(), env->team2->seeker->position);
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}

TEST(ai_test, optimal_path_long){
    auto env = setup::createEnv();
    auto path = ai::computeOptimalPath(env->team2->keeper, {0, 4}, env);
    EXPECT_EQ(path.size(), 14);
    EXPECT_EQ(path.front(), gameModel::Position(0, 4));
    EXPECT_EQ(path.back(), env->team2->keeper->position);
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}

TEST(ai_test, optimal_path_blocked){
    auto env = setup::createEnv();
    env->team1->seeker->position = {14, 11};
    env->team1->beaters[0]->position = {13, 11};
    auto path = ai::computeOptimalPath(env->team2->keeper, {15, 9}, env);
    EXPECT_EQ(path.size(), 7);
    EXPECT_EQ(path.front(), gameModel::Position(15, 9));
    EXPECT_EQ(path.back(), env->team2->keeper->position);
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}

TEST(ai_test, optimal_path_blocked_complex){
    auto env = setup::createEnv();
    env->team1->seeker->position = {9, 5};
    env->team1->chasers[0]->position = {7, 5};
    env->team1->chasers[2]->position = {9, 6};
    env->team1->beaters[0]->position = {7, 6};
    env->team2->keeper->position = {8, 7};
    auto path = ai::computeOptimalPath(env->team2->keeper, {9, 4}, env);
    EXPECT_EQ(path.size(), 5);
    EXPECT_EQ(path.front(), gameModel::Position(9, 4));
    EXPECT_EQ(path.back(), env->team2->keeper->position);
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}

TEST(ai_test, optimal_path_impossible){
    auto env = setup::createEnv();
    auto path = ai::computeOptimalPath(env->team2->seeker, env->team1->keeper->position, env);
    EXPECT_EQ(path.size(), 0);
}


//-------------------------------------eval-----------------------------------------------------------------------------

TEST(ai_test, ai_left_right_equal){
    auto env = setup::createSymmetricEnv();
    auto valLeft = ai::evalState(env, gameModel::TeamSide::LEFT, false);
    auto valRight = ai::evalState(env, gameModel::TeamSide::RIGHT, false);
    EXPECT_EQ(valLeft, valRight);
}

TEST(ai_test, ai_left_right_equal_zero){
    auto env = setup::createSymmetricEnv();
    auto valLeft = ai::evalState(env, gameModel::TeamSide::LEFT, false);
    EXPECT_EQ(valLeft, 0);
}

TEST(ai_test, ai_is_winning){
    auto env = setup::createSymmetricEnv();
    auto leftTeam = env->getTeam(gameModel::TeamSide::LEFT);
    leftTeam->score = 100;
    auto val = ai::evalState(env, gameModel::TeamSide::LEFT, false);
    EXPECT_GT(val, 0);
}

TEST(ai_test, team_has_quaffle_works_for_left){
    auto env = setup::createEnv();
    auto leftTeam = env->getTeam(gameModel::TeamSide::LEFT);
    env->quaffle->position = leftTeam->keeper->position;
    auto res = ai::teamHasQuaffle(env, leftTeam->seeker);
    EXPECT_TRUE(res);
}

TEST(ai_test, team_has_quaffle_works_for_left_when_false){
    auto env = setup::createEnv();
    auto leftTeam = env->getTeam(gameModel::TeamSide::LEFT);
    auto rightTeam = env->getTeam(gameModel::TeamSide::RIGHT);
    env->quaffle->position = rightTeam->keeper->position;
    auto res = ai::teamHasQuaffle(env, leftTeam->seeker);
    EXPECT_FALSE(res);
}

TEST(ai_test, team_has_quaffle_works_for_right){
    auto env = setup::createEnv();
    auto rightTeam = env->getTeam(gameModel::TeamSide::RIGHT);
    env->quaffle->position = rightTeam->keeper->position;
    auto res = ai::teamHasQuaffle(env, rightTeam->seeker);
    EXPECT_TRUE(res);
}

TEST(ai_test, team_has_quaffle_works_for_right_when_false){
    auto env = setup::createEnv();
    auto leftTeam = env->getTeam(gameModel::TeamSide::LEFT);
    auto rightTeam = env->getTeam(gameModel::TeamSide::RIGHT);
    env->quaffle->position = leftTeam->keeper->position;
    auto res = ai::teamHasQuaffle(env, rightTeam->seeker);
    EXPECT_FALSE(res);
}

TEST(ai_test, goal_Chance_from_neighboring_cell){
    auto env = setup::createEnv();
    auto leftTeam = env->getTeam(gameModel::TeamSide::LEFT);
    leftTeam->keeper->position = {13, 8};
    env->quaffle->position = leftTeam->keeper->position;
    auto chance = ai::getHighestGoalRate(env, leftTeam->keeper);
    EXPECT_DOUBLE_EQ(chance, env->config.gameDynamicsProbs.throwSuccess);
}

TEST(ai_test, banned_Player_has_value_zero){
    auto env = setup::createEnv();
    auto leftKeeper = env->getTeam(gameModel::TeamSide::LEFT)->keeper;
    leftKeeper->isFined = true;
    auto val = ai::evalKeeper(leftKeeper, env);
    EXPECT_DOUBLE_EQ(val, 0);
}

//------------------------------------------compute best action---------------------------------------------------------

TEST(ai_test, computeBestMove){
    using namespace communication::messages;
    auto env = setup::createEnv();
    auto playerId = types::EntityId::LEFT_CHASER3;
    auto res = ai::computeBestMove(env, playerId, false);
    EXPECT_EQ(res.getActiveEntity(), playerId);
    EXPECT_THAT(res.getDeltaType(), testing::AnyOf(types::DeltaType::MOVE, types::DeltaType::SKIP));
    if(res.getDeltaType() == types::DeltaType::MOVE){
        gameController::Move move(env, env->getPlayerById(playerId), {res.getXPosNew().value(), res.getYPosNew().value()});
        EXPECT_NE(move.check(), gameController::ActionCheckResult::Impossible);
    }
}

TEST(ai_test, computeBestShot){
    using namespace communication::messages;
    auto env = setup::createEnv({0, {}, {1, 0, 0, 0, 0}, {}});
    env->quaffle->position = env->team1->chasers[1]->position;
    auto playerId = types::EntityId::LEFT_CHASER2;
    auto res = ai::computeBestShot(env, playerId, false);
    EXPECT_EQ(res.getActiveEntity(), playerId);
    EXPECT_THAT(res.getDeltaType(), testing::AnyOf(types::DeltaType::QUAFFLE_THROW, types::DeltaType::SKIP));
    if(res.getDeltaType() == types::DeltaType::QUAFFLE_THROW){
        gameController::Shot shot(env, env->getPlayerById(playerId), env->quaffle, {res.getXPosNew().value(), res.getYPosNew().value()});
        EXPECT_NE(shot.check(), gameController::ActionCheckResult::Impossible);
    }
}

TEST(ai_test, computeBestShotBludger){
    using namespace communication::messages;
    auto env = setup::createEnv({0, {}, {1, 1, 0, 0, 0}, {}});
    env->bludgers[0]->position = env->team1->beaters[1]->position;
    auto playerId = types::EntityId::LEFT_BEATER2;
    auto res = ai::computeBestShot(env, playerId, false);
    EXPECT_EQ(res.getActiveEntity(), playerId);
    EXPECT_THAT(res.getDeltaType(), testing::AnyOf(types::DeltaType::BLUDGER_BEATING, types::DeltaType::SKIP));
    if(res.getDeltaType() == types::DeltaType::BLUDGER_BEATING){
        EXPECT_EQ(res.getPassiveEntity(), types::EntityId::BLUDGER1);
        gameController::Shot shot(env, env->getPlayerById(playerId), env->bludgers[0], {res.getXPosNew().value(), res.getYPosNew().value()});
        EXPECT_NE(shot.check(), gameController::ActionCheckResult::Impossible);
    }
}

TEST(ai_test, computeBestWrest){
    using namespace communication::messages;
    auto env = setup::createEnv({0, {}, {1, 1, 0, 0, 1}, {}});
    env->quaffle->position = env->team1->chasers[1]->position;
    env->team2->chasers[0]->position = {8, 4};
    auto playerId = types::EntityId::RIGHT_CHASER1;
    auto res = ai::computeBestWrest(env, playerId, false);
    EXPECT_EQ(res.getActiveEntity(), playerId);
    EXPECT_THAT(res.getDeltaType(), testing::AnyOf(types::DeltaType::WREST_QUAFFLE, types::DeltaType::SKIP));
    if(res.getDeltaType() == types::DeltaType::WREST_QUAFFLE){
        auto player = std::dynamic_pointer_cast<gameModel::Chaser>(env->getPlayerById(playerId));
        if(!player){
            throw std::runtime_error("No Chaser!!") ;
        }

        gameController::WrestQuaffle wrest(env, player, env->quaffle->position);
        EXPECT_NE(wrest.check(), gameController::ActionCheckResult::Impossible);
    }
}
//-------------------------------------redeploy-------------------------------------------------------------------------

TEST(ai_test, redeploy) {
    using namespace communication::messages;
    auto id = communication::messages::types::EntityId::LEFT_SEEKER;
    auto env = setup::createEnv();
    env->getPlayerById(id)->isFined = true;
    auto res = ai::redeployPlayer(env, env->getPlayerById(id)->id);
    EXPECT_EQ(res.getDeltaType(), types::DeltaType::UNBAN);
    EXPECT_EQ(res.getActiveEntity(), id);
    gameModel::Position pos{res.getXPosNew().value(), res.getYPosNew().value()};
    EXPECT_TRUE(env->cellIsFree(pos));
    EXPECT_FALSE(env->isShitOnCell(pos));
    EXPECT_NE(gameModel::Environment::getCell(pos), gameModel::Cell::GoalLeft);
    EXPECT_NE(gameModel::Environment::getCell(pos), gameModel::Cell::GoalRight);
}


//------------------------------------------Interference Useful Tests--------------------------------------------------

TEST(ai_test, getNextFanTurn0){
    using namespace communication::messages;
    auto env = setup::createEnv();
    broadcast::Next next{types::EntityId::LEFT_GOBLIN, types::TurnType::FAN, 0};
    auto deltaRequest = ai::getNextFanTurn(gameModel::TeamSide::LEFT, env, next, gameController::ExcessLength::None);
    EXPECT_THAT(deltaRequest.getDeltaType(), testing::AnyOf(types::DeltaType::SKIP, types::DeltaType::GOBLIN_SHOCK));
    if(deltaRequest.getDeltaType() == types::DeltaType::GOBLIN_SHOCK) {
        std::optional<types::EntityId> entityId = deltaRequest.getPassiveEntity();
        if(entityId.has_value()) {
            auto player = env->getPlayerById(entityId.value());
            gameController::RangedAttack rangedAttack {env, env->team2, player};
            EXPECT_TRUE(rangedAttack.isPossible());
        }
    }
}

TEST(ai_test, getNextFanTurn1){
    using namespace communication::messages;
    auto env = setup::createEnv();
    broadcast::Next next{types::EntityId::LEFT_WOMBAT, types::TurnType::FAN, 0};
    auto deltaRequest = ai::getNextFanTurn(gameModel::TeamSide::LEFT, env, next, gameController::ExcessLength::None);
    EXPECT_THAT(deltaRequest.getDeltaType(), testing::AnyOf (types::DeltaType::SKIP, types::DeltaType::WOMBAT_POO));
}

TEST(ai_test, getNextFanTurn2){
    using namespace communication::messages;
    auto env = setup::createEnv();
    broadcast::Next next{types::EntityId::LEFT_TROLL, types::TurnType::FAN, 0};
    auto deltaRequest = ai::getNextFanTurn(gameModel::TeamSide::LEFT, env, next, gameController::ExcessLength::None);
    EXPECT_THAT(deltaRequest.getDeltaType(), testing::AnyOf(types::DeltaType::SKIP, types::DeltaType::TROLL_ROAR));
    if(deltaRequest.getDeltaType() == types::DeltaType::TROLL_ROAR) {
        gameController::Impulse impulse {env, env->team2};
        EXPECT_TRUE(impulse.isPossible());
    }
}

TEST(ai_test, getNextFanTurn3){
    using namespace communication::messages;
    auto env = setup::createEnv();
    env->snitch->exists = true;
    broadcast::Next next{types::EntityId::LEFT_ELF, types::TurnType::FAN, 0};
    auto deltaRequest = ai::getNextFanTurn(gameModel::TeamSide::LEFT, env, next, gameController::ExcessLength::None);
    EXPECT_THAT(deltaRequest.getDeltaType(), testing::AnyOf(types::DeltaType::SKIP, types::DeltaType::ELF_TELEPORTATION));
    if(deltaRequest.getDeltaType() == types::DeltaType::ELF_TELEPORTATION) {
        auto entityId = deltaRequest.getPassiveEntity();
        if(entityId.has_value()) {
            auto player = env->getPlayerById(entityId.value());
            gameController::Teleport teleport {env, env->team2, player};
            EXPECT_TRUE(teleport.isPossible());
            EXPECT_TRUE(env->getTeam(player)->side == gameModel::TeamSide::RIGHT);
        }
    }
}

TEST(ai_test, getNextFanTurn4){
    using namespace communication::messages;
    auto env = setup::createEnv();
    env->snitch->exists = true;
    broadcast::Next next{types::EntityId::LEFT_NIFFLER, types::TurnType::FAN, 0};
    auto deltaRequest = ai::getNextFanTurn(gameModel::TeamSide::LEFT, env, next, gameController::ExcessLength::None);
    EXPECT_THAT(deltaRequest.getDeltaType(), testing::AnyOf(types::DeltaType::SKIP, types::DeltaType::SNITCH_SNATCH));
    if(deltaRequest.getDeltaType() == communication::messages::types::DeltaType::SNITCH_SNATCH) {
        gameController::SnitchPush snitchPush {env, env->team2};
        EXPECT_TRUE(snitchPush.isPossible());
    }
}