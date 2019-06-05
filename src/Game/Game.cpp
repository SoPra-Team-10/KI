/**
 * @file Game.cpp
 * @author paul
 * @date 02.06.19
 * @brief Game @TODO
 */

#include "Game.hpp"

Game::Game(unsigned int ) {

}

auto Game::getTeamFormation() -> communication::messages::request::TeamFormation {
    return {};
}

void Game::onSnapshot(const communication::messages::broadcast::Snapshot &snapshot) {

}

auto Game::getNextAction(const communication::messages::broadcast::Next &next)
    -> communication::messages::request::DeltaRequest {
    return communication::messages::request::DeltaRequest();
}

void Game::setMatchStart(const communication::messages::broadcast::MatchStart &matchStart) {

}

