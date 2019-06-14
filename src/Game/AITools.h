#include <utility>

//
// Created by tim on 14.06.19.
//

#ifndef KI_AITOOLS_H
#define KI_AITOOLS_H

#include <memory>
#include <optional>
#include <functional>

namespace aiTools{
    template <typename T>
    class SearchNode {
    public:
        SearchNode(const std::shared_ptr<T> &state, std::optional<std::shared_ptr<T>> parent, int pathCost,
                   std::function<int()> f)
                : state(state), parent(parent), pathCost(pathCost), f(std::move(f)) {}

        std::shared_ptr<T> state;
        std::optional<std::shared_ptr<T>> parent;
        int pathCost = 0;
        std::function<int()> f;
    };
}

#endif //KI_AITOOLS_H
