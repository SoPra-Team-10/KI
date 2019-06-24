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
     * @return the next action by the AI or nothing if not AIs turn
     */
    auto getNextAction(const communication::messages::broadcast::Next &next)
        -> std::optional<communication::messages::request::DeltaRequest>;

private:
    static constexpr int FIELD_WIDTH = 16;
    int difficulty;
    int currentRound = 1;
    communication::messages::types::PhaseType currentPhase = communication::messages::types::PhaseType::BALL_PHASE;
    bool goalScoredThisRound = false;
    std::optional<std::shared_ptr<gameModel::Environment>> currentEnv = std::nullopt;
    gameModel::TeamSide mySide;
    communication::messages::request::TeamConfig myConfig;
    communication::messages::request::TeamConfig theirConfig = {};
    communication::messages::broadcast::MatchConfig matchConfig = {};
    std::vector<communication::messages::types::EntityId> usedPlayersOwn = {};
    std::vector<communication::messages::types::EntityId> usedPlayersOpponent = {};


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
