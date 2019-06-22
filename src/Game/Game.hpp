/**
 * @file Game.hpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#ifndef KI_GAME_HPP
#define KI_GAME_HPP

#include <SopraMessages/TeamFormation.hpp>
#include <SopraMessages/MatchStart.hpp>
#include <SopraMessages/Snapshot.hpp>
#include <SopraMessages/Next.hpp>
#include <SopraMessages/DeltaRequest.hpp>
#include <SopraGameLogic/GameModel.h>


class Game {
public:
    Game(unsigned int difficulty, communication::messages::request::TeamConfig ownTeamConfig);

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
     * @return the next action by the AI
     */
    auto getNextAction(const communication::messages::broadcast::Next &next)
        -> communication::messages::request::DeltaRequest;

private:
    static constexpr int FIELD_WIDTH = 16;
    int difficulty;
    int currentRound = 1;
    std::optional<std::shared_ptr<gameModel::Environment>> currentEnv = std::nullopt;
    gameModel::TeamSide side;
    communication::messages::request::TeamConfig myConfig;
    communication::messages::request::TeamConfig theirConfig = {};
    communication::messages::broadcast::MatchConfig matchConfig = {};

    /**
     * Constructs a Team object from a given TeamSnapshot
     * @param teamSnapshot
     * @param teamSide side on which the Team plays
     * @return gameModel::Team
     */
    auto teamFromSnapshot(const communication::messages::broadcast::TeamSnapshot &teamSnapshot, gameModel::TeamSide teamSide) const ->
        std::shared_ptr<gameModel::Team>;

    /**
     * Mirrors the given position in place on the x-axis
     * @param pos
     */
    void mirrorPos(gameModel::Position &pos) const;
};


#endif //KI_GAME_HPP
