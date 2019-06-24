//
// Created by tarik on 10.06.19.
//

#ifndef KI_AI_H
#define KI_AI_H

#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraMessages/Message.hpp>

namespace ai{
    int evalState();

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
     * evaluates if it is necessary to use a fan
     * @param env is the actual Environment
     * @param next gives the next EntityID
     * @return returns the new DeltaRequest
     */
    auto getNextFanTurn(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env, communication::messages::broadcast::Next &next)->const communication::messages::request::DeltaRequest;

    bool isNifflerUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env);

    auto isElfUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env, const gameController::ExcessLength &excessLength) -> const std::optional<communication::messages::types::EntityId>;

    bool isTrollUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env);

    auto getGoblinTarget(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env) -> const std::optional<communication::messages::types::EntityId>;

    auto getDistance(const gameModel::Position &startPoint, const gameModel::Position &endPoint) -> int;
}

#endif //KI_AI_H

