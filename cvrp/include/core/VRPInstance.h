#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include "core/Node.h"

struct VRPInstance {
    int depot_id  = 0;
    int capacity  = 0;
    int num_trucks = 0;
    int optimal_value = 0;
    std::vector<Node> nodes;
    std::string name;
    std::unordered_map<int, size_t> mapa;
    std::vector<size_t> indicePorId;
    std::vector<std::vector<int>> distancias;
    static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

    const Node& getDepot() const {
        return getNode(depot_id);
    }

    void buildIndex() {
        mapa.clear();
        indicePorId.clear();
        distancias.clear();

        int maxId = 0;
        for (size_t i = 0; i < nodes.size(); i++) {
            maxId = std::max(maxId, nodes[i].id);
        }

        indicePorId.assign(static_cast<size_t>(maxId) + 1, invalid_index);

        for (size_t i = 0; i < nodes.size(); i++) {
            mapa[nodes[i].id] = i;
            if (nodes[i].id >= 0) {
                indicePorId[static_cast<size_t>(nodes[i].id)] = i;
            }
        }

        distancias.assign(nodes.size(), std::vector<int>(nodes.size(), 0));
        for (size_t i = 0; i < nodes.size(); i++) {
            for (size_t j = i + 1; j < nodes.size(); j++) {
                int dx = nodes[i].x - nodes[j].x;
                int dy = nodes[i].y - nodes[j].y;
                int distancia = static_cast<int>(std::round(std::sqrt(dx*dx + dy*dy)));
                distancias[i][j] = distancia;
                distancias[j][i] = distancia;
            }
        }
    }

    const Node& getNode(int id) const{
        return nodes[indice(id)];
    }

    int distancia(const Node& a, const Node& b) const {
        if (!distancias.empty()) {
            return distancias[indice(a.id)][indice(b.id)];
        }

        int dx = a.x - b.x;
        int dy = a.y - b.y;
        return static_cast<int>(std::round(std::sqrt(dx*dx + dy*dy)));
    }

    size_t indice(int id) const {
        if (id >= 0) {
            size_t idx = static_cast<size_t>(id);
            if (idx < indicePorId.size() && indicePorId[idx] != invalid_index) {
                return indicePorId[idx];
            }
        }

        return mapa.at(id);
    }
};
