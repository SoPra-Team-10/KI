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

class Game {
public:
    explicit Game(unsigned int difficulty);
    auto getTeamFormation() -> communication::messages::request::TeamFormation;
    void setMatchStart(const communication::messages::broadcast::MatchStart &matchStart);
    void onSnapshot(const communication::messages::broadcast::Snapshot &snapshot);
    auto getNextAction(const communication::messages::broadcast::Next &next)
        -> communication::messages::request::DeltaRequest;
};


#endif //KI_GAME_HPP
