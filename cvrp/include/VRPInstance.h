#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include "Node.h"

struct VRPInstance {
    int depot_id  = 0;
    int capacity  = 0;
    int num_trucks = 0;
    std::vector<Node> nodes;
    std::string name;

    const Node& getDepot() const {
        for (const auto& node : nodes) {
            if (node.id == depot_id) return node;
        }
        throw std::runtime_error("Depósito não encontrado");
    }

    double distancia(const Node& a, const Node& b) const {
        int dx = a.x - b.x;
        int dy = a.y - b.y;
        return std::round(std::sqrt(dx*dx + dy*dy));
    }
};
