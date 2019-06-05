/**
 * @file Game.cpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#include "Game.hpp"

Game::Game(unsigned int , communication::messages::request::TeamConfig &) {}

auto Game::getTeamFormation(const communication::messages::broadcast::MatchStart &)
    -> communication::messages::request::TeamFormation {
    return {};
}

void Game::onSnapshot(const communication::messages::broadcast::Snapshot &) {

}

auto Game::getNextAction(const communication::messages::broadcast::Next &)
    -> communication::messages::request::DeltaRequest {
    return communication::messages::request::DeltaRequest();
}

