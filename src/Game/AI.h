//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include "Game.hpp"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraMessages/Message.hpp>
#include <SopraUtil/Logging.hpp>

namespace ai {

    double simpleEval(const aiTools::State &state, gameModel::TeamSide mySide);

    /**
     * Evaluates a game situation
     * @param env Environment to evaluate
     * @param mySide The Side that the KI is playing
     * @param goalScoredThisRound Whether or not a goal was scored this round
     * @return A number indicating how favorable the current situation is. The higher the number the better
     */

    double evalState(const std::shared_ptr<const gameModel::Environment> &environment, gameModel::TeamSide mySide,
                     bool goalScoredThisRound);

    /**
     * Evaluates the positioning of players in a single team
     * @param team The team to be evaluated
     * @param env The environment the team is playing in
     * @return A number indicating the value of the team
     */
    double evalTeam(const std::shared_ptr<const gameModel::Team> &team, const std::shared_ptr<gameModel::Environment> &env);

    /**
     * Evaluates the positioning of a seeker
     * @param seeker Seeker to be evaluated
     * @param env Environment the seeker is in
     * @return A number that indicates the value of the seeker
     */
    double evalSeeker(const std::shared_ptr<const gameModel::Seeker> &seeker,
                      const std::shared_ptr<const gameModel::Environment> &env);
    /**
     * Evaluates the positioning of a keeper
     * @param keeper Keeper to be evaluated
     * @param env Environment the keeper is in
     * @return A number that indicates the value of the keeper
     */
    double evalKeeper(const std::shared_ptr<gameModel::Keeper> &keeper, const std::shared_ptr<gameModel::Environment> &env);

    /**
     * Evaluates the positioning of a chaser
     * @param chaser Chaser to be evaluated
     * @param env Environment the chaser is in
     * @return A number that indicates the value of the chaser
     */
    double evalChaser(const std::shared_ptr<gameModel::Chaser> &chaser,
                      const std::shared_ptr<gameModel::Environment> &env);

    /**
     * Evaluates the positioning of the bludgers relative to the players
     * @param env The environment to be evaluated
     * @param mySide The side the KI is playing
     * @return A number indicating how good the positions of the bluders are for the KI
     */
    double evalBludgers(const std::shared_ptr<const gameModel::Environment> &env, gameModel::TeamSide mySide);

    /**
     * Calculates the chance of an actor to score a goal in any enemy goal ring
     * @param env The environment where the shots happen
     * @param actor The player that wants to shoot the quaffle
     * @return The highest success rate of any shot resulting in a goal
     */
    double getHighestGoalRate(const std::shared_ptr<gameModel::Environment> &env,
            const std::shared_ptr<gameModel::Player> &actor);

    /**
     * Checks if the team of the given Player has the Quaffle
     * @param env the current Environment
     * @param player The Player of the Team to be checked
     * @return true if the player's Team is in possession of the Quaffle, false otherwise
     */
    bool teamHasQuaffle(const std::shared_ptr<const gameModel::Environment> &env, const std::shared_ptr<const gameModel::Player> &player);

    /**
     * Calculates the theoretical chance of scoring a goal from the current position of the Quaffle
     * @param env the Environment to operate on
     * @param teamSide side of the team to attempt the shot
     * @return highest chance of scoring a goal if a non existing player of playing for the team held the Quaffle
     */
    double hypotheticalShotSuccessProb(const std::shared_ptr<gameModel::Environment> &env, gameModel::TeamSide teamSide);

}

#endif //KI_AI_H

