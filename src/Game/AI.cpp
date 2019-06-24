//
// Created by tarik on 10.06.19.
//

#include "AI.h"
#include "AITools.h"
#include <SopraGameLogic/GameController.h>

namespace ai {
    int evalState() {
        int val = 0;

        //TODO: Make this a useful function

        return val;
    }

    auto
    computeOptimalPath(const std::shared_ptr<const gameModel::Player> &player, const gameModel::Position &destination,
                       const std::shared_ptr<const gameModel::Environment> &env) -> std::vector<gameModel::Position> {
        if (player->position == destination) {
            return {};
        }

        auto expandFunction = [&env, &player](const aiTools::SearchNode<gameModel::Position> &node) {
            std::vector<aiTools::SearchNode<gameModel::Position>> path;
            path.reserve(8);
            auto parent = std::make_shared<aiTools::SearchNode<gameModel::Position>>(node);
            for (const auto &pos : env->getAllLegalCellsAround(node.state, env->team1->hasMember(player))) {
                path.emplace_back(pos, parent, node.pathCost + 1);
            }

            return path;
        };

        auto evalFunction = [&destination](const aiTools::SearchNode<gameModel::Position> &pos) {
            int g = pos.parent.has_value() ? pos.parent.value()->pathCost + 1 : 1;
            return g + gameController::getDistance(pos.state, destination);
        };

        aiTools::SearchNode<gameModel::Position> startNode(player->position, std::nullopt, 0);
        aiTools::SearchNode<gameModel::Position> destinationNode(destination, std::nullopt, 0);
        auto res = aiTools::aStarSearch(startNode, destinationNode, expandFunction, evalFunction);
        std::vector<gameModel::Position> ret;
        if (res.has_value()) {
            ret.reserve(res->pathCost + 1);
            ret.emplace_back(res->state);
            auto parent = res->parent;
            while (parent.has_value()) {
                ret.emplace_back((*parent)->state);
                parent = (*parent)->parent;
            }
        }

        return ret;
    }

    auto getAllCrossedCells(const gameModel::Position &startPoint, const gameModel::Position &endPoint) ->
    std::vector<gameModel::Position> {

        std::vector<gameModel::Position> resultVect;

        // check if cells are valid
        if (gameModel::Environment::getCell(startPoint) == gameModel::Cell::OutOfBounds ||
            gameModel::Environment::getCell(endPoint) == gameModel::Cell::OutOfBounds){
            throw std::out_of_range("Source or destination of movement vector are out of bounds");
        }

        // check if start and end point are equal
        if (startPoint == endPoint)
            return resultVect;

        // define an normalize the direction vector
        gameModel::Vector dirVect(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
        dirVect.normalize();

        // traverse the route between the two points
        gameModel::Vector travVect(0, 0);
        gameModel::Position lastCell = startPoint;
        while ((travVect + startPoint) != endPoint) {

            // found a new crossed cell
            if ((travVect + startPoint) != lastCell) {
                lastCell = travVect + startPoint;
                resultVect.emplace_back(lastCell);
            }

            // make a step to travers the vector
            travVect = travVect + (dirVect * 0.5);
        }

        return resultVect;
    }

    auto getDistance(const gameModel::Position &startPoint, const gameModel::Position &endPoint) -> int {

        // check if cells are valid
        if (gameModel::Environment::getCell(startPoint) == gameModel::Cell::OutOfBounds ||
            gameModel::Environment::getCell(endPoint) == gameModel::Cell::OutOfBounds){

            return -1;
        }

        // check if start and end point are equal
        if (startPoint == endPoint){
            return 0;
        }

        int totalDistance = 0;

        // calc the differences within the components of the given points
        int dX = std::abs(startPoint.x - endPoint.x);
        int dY = std::abs(startPoint.y - endPoint.y);

        // calculate the total distance
        if (dX >= dY) {
            totalDistance += dY;
            totalDistance += dX-dY;
        } else {
            totalDistance += dX;
            totalDistance += dY-dX;
        }

        return totalDistance;
    }

    auto getNextFanTurn(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env,
                        communication::messages::broadcast::Next &next) -> const communication::messages::request::DeltaRequest {
        communication::messages::types::EntityId activeEntityId = next.getEntityId();
        std::optional<communication::messages::types::EntityId> passiveEntityId;
        if (activeEntityId == communication::messages::types::EntityId::LEFT_NIFFLER ||
            activeEntityId == communication::messages::types::EntityId::RIGHT_NIFFLER) {
            if(isNifflerUseful(mySide, env)){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SNITCH_SNATCH, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_ELF ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_ELF){
            passiveEntityId = isElfUseful(mySide, env);
            if(passiveEntityId.has_value()){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SNITCH_SNATCH, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_TROLL ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_TROLL){
            if(isTrollUseful(mySide, env)){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::TROLL_ROAR, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_GOBLIN ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_GOBLIN){
            passiveEntityId = isGoblinUseful(mySide, env);
            if(passiveEntityId.has_value()){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::GOBLIN_SHOCK, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, std::nullopt, passiveEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        }else if(activeEntityId == communication::messages::types::EntityId::LEFT_WOMBAT ||
                 activeEntityId == communication::messages::types::EntityId::RIGHT_WOMBAT){
            std::optional<std::vector<int, int>> newPosition;
            if(newPosition.has_value()){
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::WOMBAT_POO, std::nullopt, std::nullopt, std::nullopt, newPosition[0], newPosition[1],
                                                                      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }else{
                return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                      std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            }
        } else{
            return communication::messages::request::DeltaRequest{communication::messages::types::DeltaType::SKIP, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                  std::nullopt, activeEntityId, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        }
    }

    bool isNifflerUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env){
        if (mySide == env->team1->side) {
            return getDistance(env->team2->seeker->position, env->snitch->position) <= 2;
        }else{
            return getDistance(env->team1->seeker->position, env->snitch->position) <= 2;
        }
    }

    bool isTrollUseful(const gameModel::TeamSide &mySide, const std::shared_ptr<gameModel::Environment> &env) {
        if (mySide == env->team1->side) {
            for(const auto &chase : env->team2->chasers){
                if(chase->position == env->quaffle->position){
                    return true;
                }
            }
        }else {
            for(const auto &chase : env->team1->chasers){
                if(chase->position == env->quaffle->position){
                    return true;
                }
            }
        }
        return false;
    }

    auto isGoblinUseful(const gameModel::TeamSide &mySide,
                        const std::shared_ptr<gameModel::Environment> &env) -> std::optional<communication::messages::types::EntityId> {
        if (mySide == env->team1->side) {
            for(const auto &chase : env->team2->chasers){
                if(chase->position == env->quaffle->position){
                    return chase->id;
                }
            }
        }else {
            for(const auto &chase : env->team1->chasers){
                if(chase->position == env->quaffle->position){
                    return chase->id;
                }
            }
        }
        return std::nullopt;
    }

    auto isElfUseful(const gameModel::TeamSide &mySide,
                     const std::shared_ptr<gameModel::Environment> &env) -> std::optional<communication::messages::types::EntityId> {
        return std::optional<communication::messages::types::EntityId>();
    }
}