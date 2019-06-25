//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include "Game.hpp"
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraMessages/Message.hpp>
namespace ai {

    /**
     * Evaluates a game situation
     * @param env Environment to evaluate
     * @param mySide The Side that the KI is playing
     * @param goalScoredThisRound Wheather or not a goal was scored this round
     * @return A number indicating how favorable the current situation is. The higher the number the better
     */
    double evalState(const std::shared_ptr<const gameModel::Environment> &environment, gameModel::TeamSide mySide, bool goalScoredThisRound);

    /**
     * Evaluates the positioning of players in a single team
     * @param team The team to be evaluated
     * @param env The environment the team is playing in
     * @return A number indicating the value of the team
     */
    double evalTeam(const std::shared_ptr<const gameModel::Team> &team,
                    const std::shared_ptr<gameModel::Environment> &env);

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
    double evalKeeper(const std::shared_ptr<gameModel::Keeper> &keeper,
                      const std::shared_ptr<gameModel::Environment> &env);

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
     * Computes the optimal path from start to destination avoiding possible fouls using A*-search
     * @param start The Position to start
     * @param destination The desired gol Position
     * @param env The Environment to operate on
     * @return The optimal path as a list of Positions ordered from start (vector::back) to destination (vector::front).
     * Start an destination are inclusivie. Empty if start = destination or when no path exists
     */
    auto computeOptimalPath(const std::shared_ptr<const gameModel::Player> &player, const gameModel::Position &destination,
                            const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position>;

    /**
     * Computes the next move according to the current state of the game
     * @param env the current game state
     * @param id the ID of the player to make a move
     * @param goalScoredThisRound indicates whether a goal was scored in the current round
     * @throws std::runtime_error when no move is possible
     * @return next move as DeltaRequest
     */
    auto computeBestMove(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
            bool goalScoredThisRound) -> communication::messages::request::DeltaRequest;

    /**
     * Computes the next shot according to the current state of the game
     * @param env the current game state
     * @param id the ID of the player to perform a shot
     * @param goalScoredThisRound indicates whether a goal was scored in the current round
     * @throws std::runtime_error when no shot is possible
     * @return next shot as DeltaRequest
     */
    auto computeBestShot(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
            bool goalScoredThisRound) -> communication::messages::request::DeltaRequest;


    /**
     * Computes the next wrest according to the current state of the game
     * @param env the current game state
     * @param id the ID of the player to perform a shot
     * @param goalScoredThisRound indicates whether a goal was scored in the current round
     * @throws std::runtime_error when no shot is possible
     * @return next wrest as DeltaRequest
     */
    auto computeBestWrest(const std::shared_ptr<gameModel::Environment> &env, communication::messages::types::EntityId id,
            bool goalScoredThisRound) -> communication::messages::request::DeltaRequest;

    /**
     * Computes the Position where the banned Player should be redeployed
     * @param env the current game state
     * @param id the ID of the player to perform a shot
     * @return unban request as DeltaRequest
     */
    auto redeployPlayer(const std::shared_ptr<const gameModel::Environment> &env, communication::messages::types::EntityId id)
        -> communication::messages::request::DeltaRequest;

    /**
     * evaluates if it is necessary to use a fan
     * @param env is the actual Environment
     * @param next gives the next EntityID
     * @return returns the new DeltaRequest
     */
    auto getNextFanTurn(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
            const communication::messages::broadcast::Next &next, const gameController::ExcessLength &excessLength)
                        -> const communication::messages::request::DeltaRequest;

    /**
     * evaluates, if it is useful to use a Niffler as a Fan
     * @param mySide is the Teamside from the AI
     * @param env ist the current Environment
     * @return true, if it is useful, to use a Niffler, otherwise false
     */
    bool isNifflerUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env);

    /**
     * evaluates, if it is useful to use an Elf as a Fan
     * @param mySide  is the Teamside from the AI
     * @param env is the current Environment
     * @param excessLength from the Snitch is needed, to act to the special behavior  from the Snitch
     * @return returns an EntityID from the Target, which should be teleported
     */
    auto getElfTarget(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env,
                      const gameController::ExcessLength &excessLength) -> const std::optional<communication::messages::types::EntityId>;

    /**
    * evaluates, if it is useful to use a Troll as a Fan
    * @param mySide is the Teamside from the AI
    * @param env ist the current Environment
    * @return true, if it is useful, to use a Troll, otherwise false
    */
    bool isTrollUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env);

    /**
    * evaluates, if it is useful to use a Goblin as a Fan
    * @param mySide is the Teamside from the AI
    * @param env ist the current Environment
    * @return returns an EntityID from the Target, which be attacked by the Range-Attack
    */
    auto getGoblinTarget(const gameModel::TeamSide &mySide, const std::shared_ptr<const gameModel::Environment> &env) -> const std::optional<communication::messages::types::EntityId>;
}

#endif //KI_AI_H

