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
    enum class TeamSide{
        LEFT, RIGHT;
    };

public:
    explicit Game(unsigned int difficulty);

    /**
     * Gets the TeamFormation for the match
     * @return
     */
    auto getTeamFormation() -> communication::messages::request::TeamFormation;

    /**
     * Constructs the first internal game state from the given broadcast
     * @param matchStart The message received at the beginning of the match
     */
    void setMatchStart(const communication::messages::broadcast::MatchStart &matchStart);

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
    std::shared_ptr<gameModel::Environment> currentEnv;
    TeamSide side;
};


#endif //KI_GAME_HPP
