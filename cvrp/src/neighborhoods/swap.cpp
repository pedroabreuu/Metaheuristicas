#include <vector>
#include <algorithm>
#include <limits>
#include "neighborhoods/swap.h"

Solution swapIntra(Solution solucao, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();
    const auto& nodes = instance.nodes;
    const auto& mapa = instance.mapa;

    auto nodeById = [&](int id) -> const Node& {
        return nodes[mapa.at(id)];
    };

    bool melhorou = true;

    while (melhorou) {
        melhorou = false;

        double bestDelta = 0.0;
        int bestRota = -1;
        int bestJ = -1;
        int bestK = -1;

        for (size_t r = 0; r < solucao.rotas.size(); r++) {
            auto& rota = solucao.rotas[r];
            if (rota.size() < 2) continue;

            for (size_t j = 0; j + 1 < rota.size(); j++) {
                for (size_t k = j + 1; k < rota.size(); k++) {
                    const int uId = rota[j];
                    const int vId = rota[k];

                    const Node& u = nodeById(uId);
                    const Node& v = nodeById(vId);

                    double antes = 0.0, depois = 0.0, delta = 0.0;

                    if (k == j + 1) { // estao adjacentes
                        const Node& a = (j == 0) ? depot : nodeById(rota[j - 1]);
                        const Node& d = (k + 1 == rota.size()) ? depot : nodeById(rota[k + 1]);

                        antes = instance.distancia(a, u) + instance.distancia(u, v) + instance.distancia(v, d);
                        depois = instance.distancia(a, v) + instance.distancia(v, u) + instance.distancia(u, d);

                    } else {
                        const Node& a = (j == 0) ? depot : nodeById(rota[j - 1]);
                        const Node& b = (j + 1 == rota.size()) ? depot : nodeById(rota[j + 1]);
                        const Node& c = (k == 0) ? depot : nodeById(rota[k - 1]);
                        const Node& d = (k + 1 == rota.size()) ? depot : nodeById(rota[k + 1]);

                        antes = instance.distancia(a, u) + instance.distancia(u, b) + instance.distancia(c, v) + instance.distancia(v, d);
                        depois = instance.distancia(a, v) + instance.distancia(v, b) + instance.distancia(c, u) + instance.distancia(u, d);
                    }

                    delta = depois - antes;
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestRota = static_cast<int>(r);
                        bestJ = static_cast<int>(j);
                        bestK = static_cast<int>(k);
                    }
                }
            }
        }

        if (bestRota != -1) {
            std::swap(solucao.rotas[bestRota][bestJ], solucao.rotas[bestRota][bestK]);
            melhorou = true;
        }
    }

    return solucao;
}