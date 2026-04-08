#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include "Node.h"

struct VRPInstance {
    int depot_id  = 0;
    int capacity  = 0;
    int num_trucks = 0;
    int optimal_value = 0;
    std::vector<Node> nodes;
    std::string name;
    std::unordered_map<int, size_t> mapa;

    const Node& getDepot() const {
        return getNode(depot_id);
    }

    void buildIndex() {
        for (size_t i = 0; i < nodes.size(); i++) {
            mapa[nodes[i].id] = i;
        }
    }

    const Node& getNode(int id) const{
        return nodes[mapa.at(id)];
    }

    double distancia(const Node& a, const Node& b) const {
        int dx = a.x - b.x;
        int dy = a.y - b.y;
        return std::round(std::sqrt(dx*dx + dy*dy));
    }
};
