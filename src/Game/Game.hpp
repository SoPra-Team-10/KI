/**
 * @file Game.hpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#ifndef KI_GAME_HPP
#define KI_GAME_HPP

#include <unordered_set>

#include <SopraMessages/TeamFormation.hpp>
#include <SopraMessages/MatchStart.hpp>
#include <SopraMessages/Snapshot.hpp>
#include <SopraMessages/Next.hpp>
#include <SopraMessages/DeltaRequest.hpp>
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraAITools/AITools.h>
#include <Mlp/Mlp.hpp>

class Game {
public:
    Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig,
            const std::string &mlpFName);

    /**
     * Gets the TeamFormation for the match
     * @return
     */
    auto getTeamFormation(const communication::messages::broadcast::MatchStart &matchStart) -> communication::messages::request::TeamFormation;

    /**
     * Updates the game state after a broadcast
     * @param snapshot the current game state from the server
     */
    void onSnapshot(const communication::messages::broadcast::Snapshot &snapshot);

    /**
     * Returns the AIs next action
     * @param next information from the server for the requested turn
     * @return the next action by the AI or nothing if not AIs turn
     */
    auto getNextAction(const communication::messages::broadcast::Next &next)
        -> std::optional<communication::messages::request::DeltaRequest>;

private:
    static constexpr int FIELD_WIDTH = 16;
    int difficulty;
    bool gotFirstSnapshot = false;
    aiTools::State currentState;
    gameModel::TeamSide mySide;
    communication::messages::request::TeamConfig myConfig;
    communication::messages::request::TeamConfig theirConfig = {};
    communication::messages::broadcast::MatchConfig matchConfig = {};

    ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1> stateEstimator;


    /**
     * Constructs a Team object from a given TeamSnapshot
     * @param teamSnapshot
     * @param teamSide side on which the Team plays
     * @return gameModel::Team
     */
    auto teamFromSnapshot(const communication::messages::broadcast::TeamSnapshot &teamSnapshot, gameModel::TeamSide teamSide) const ->
        std::shared_ptr<gameModel::Team>;
};


#endif //KI_GAME_HPP
