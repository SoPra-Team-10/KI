/**
 * @file Game.hpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#ifndef KI_GAME_HPP
#define KI_GAME_HPP

#include <SopraMessages/TeamFormation.hpp>

class Game {
public:
    explicit Game(unsigned int difficulty);
    auto getTeamFormation() -> communication::messages::request::TeamFormation;
};


#endif //KI_GAME_HPP
