//
// Created by tim on 16.06.19.
//

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <Game/AI.h>
#include "setup.h"
#include <SopraGameLogic/GameController.h>


TEST(ai_test, optimal_path){
    auto env = setup::createEnv();
    auto path = ai::computeOptimalPath(env->team2->seeker, {8, 6}, env);
    EXPECT_EQ(path.size(), 4);
    EXPECT_EQ(path.front(), gameModel::Position(8, 6));
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}

TEST(ai_test, optimal_path_long){
    auto env = setup::createEnv();
    auto path = ai::computeOptimalPath(env->team2->keeper, {0, 4}, env);
    EXPECT_EQ(path.size(), 14);
    EXPECT_EQ(path.front(), gameModel::Position(0, 4));
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
    for(unsigned long i = 1; i < path.size(); i++){
        EXPECT_EQ(gameController::getDistance(path[i - 1], path[i]), 1);
    }
}
