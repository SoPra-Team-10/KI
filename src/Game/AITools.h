
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

namespace aiTools{
    template <typename T>
    class SearchNode {
    public:
        SearchNode(T state, std::optional<std::shared_ptr<SearchNode<T>>> parent, int pathCost) :
            state(state), parent(parent), pathCost(pathCost) {}

        T state;
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
    template <typename T, typename EXP, typename F>
    auto aStarSearch(SearchNode<T> &start, SearchNode<T> &destination,
            EXP &expand, F &f) -> std::optional<SearchNode<T>>{
        std::deque<SearchNode<T>> visited;
        std::list<SearchNode<T>> fringe = {start};
        std::optional<SearchNode<T>> ret;
        while(!fringe.empty()){
            const auto currentNode = *fringe.begin();
            fringe.pop_front();
            if(currentNode == destination){
                ret = currentNode;
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
}

#endif //KI_AITOOLS_H
