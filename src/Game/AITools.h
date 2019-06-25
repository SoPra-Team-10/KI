
//
// Created by tim on 14.06.19.
//

#ifndef KI_AITOOLS_H
#define KI_AITOOLS_H

#include <memory>
#include <optional>
#include <deque>
#include <list>
#include <functional>
#include <set>
#include <SopraGameLogic/Action.h>

namespace aiTools{
    template <typename T>
    class SearchNode {
    public:
        SearchNode(T state, std::optional<std::shared_ptr<SearchNode<T>>> parent, int pathCost) :
            state(std::move(state)), parent(std::move(parent)), pathCost(pathCost) {}

        const T state;
        std::optional<std::shared_ptr<SearchNode<T>>> parent;
        int pathCost = 0;
        bool operator==(const SearchNode<T> &other) const;
    };


    template<typename T>
    bool SearchNode<T>::operator==(const SearchNode<T> &other) const {
        return this->state == other.state;
    }


    /**
     * A*algorithm implemented as graph search
     * @tparam T Type of the SearchNode used
     * @tparam EXP Type of expand function used, must return iterable of SearchNodes<T>
     * @tparam F Type of evaluation function for nodes. Must return comparable value
     * @param Start initial state
     * @param Destination goal state
     * @param expand Function used for expanding nodes
     * @param f Function used for evaluating a node. A high value corresponds to low utility
     * @return The goal node with information about its parent or nothing if problem is impossible
     */
    template <typename T, typename Exp, typename F>
    auto aStarSearch(const SearchNode<T> &start, const SearchNode<T> &destination,
            const Exp &expand, const F &f) -> std::optional<SearchNode<T>>{
        std::deque<SearchNode<T>> visited;
        std::list<SearchNode<T>> fringe = {start};
        std::optional<SearchNode<T>> ret;
        while(!fringe.empty()){
            const auto currentNode = *fringe.begin();
            fringe.pop_front();
            if(currentNode == destination){
                ret.emplace(currentNode);
                break;
            }

            bool nodeVisited = false;
            for(const auto &node : visited){
                if(currentNode == node){
                    nodeVisited = true;
                    break;
                }
            }

            if(nodeVisited){
                continue;
            }

            visited.emplace_back(currentNode);
            for(const auto &newNode : expand(currentNode)){
                bool inserted = false;
                for(auto it = fringe.begin(); it != fringe.end(); ++it){
                    if(f(newNode) < f(*it)){
                        fringe.emplace(it, newNode);
                        inserted = true;
                        break;
                    }
                }

                if(!inserted){
                    fringe.emplace_back(newNode);
                    fringe.size();
                }
            }
        }

        return ret;
    }

    /**
     * Selects the best Action from a list of Actions.
     * @tparam ActionType type of Action
     * @tparam EvalFun type of function used for evaluating the actions' outcomes
     * @param actionList list of actions to evaluate
     * @param evalFun evaluation function for comparing outcomes of actions
     * @return tuple of the best Action and its score
     */
    template <typename ActionType, typename EvalFun>
    auto chooseBestAction(const std::vector<ActionType> &actionList, const EvalFun &evalFun) ->
        std::tuple<typename std::vector<ActionType>::const_iterator, double>{
        if(actionList.empty()){
            throw std::runtime_error("List is empty. Cannot choose best entry");
        }

        double highestScore = -std::numeric_limits<double>::infinity();
        std::optional<typename std::vector<ActionType>::const_iterator> best;
        for(auto action = actionList.begin(); action < actionList.end(); ++action){
            double tmpScore = 0;
            for(const auto &outcome : action->executeAll()){
                tmpScore += outcome.second * evalFun(outcome.first);
            }

            if(tmpScore > highestScore){
                highestScore = tmpScore;
                best.emplace(action);
            }
        }

        if(!best.has_value()){
            throw std::runtime_error("No best candidate found");
        }

        return {best.value(), highestScore};
    }
}

#endif //KI_AITOOLS_H