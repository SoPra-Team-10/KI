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
