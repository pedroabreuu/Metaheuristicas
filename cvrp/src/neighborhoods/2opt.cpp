#include "utils/rng.h"
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include "neighborhoods/2opt.h"

Solution opt2(Solution solucao, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();

    auto nodeById = [&](int id) -> const Node& {
        return instance.getNode(id);
    };

    for (size_t i = 0; i < solucao.rotas.size(); i++) {
        if (solucao.rotas[i].size() < 2) continue;

        bool melhorou = true;
        while (melhorou) {
            melhorou = false;

            double bestDelta = 0.0;
            int bestJ = -1;
            int bestK = -1;

            for (size_t j = 0; j + 1 < solucao.rotas[i].size(); j++) {
                for (size_t k = j + 1; k < solucao.rotas[i].size(); k++) {
                    const Node& noA = (j == 0) ? depot : nodeById(solucao.rotas[i][j - 1]);
                    const Node& noB = nodeById(solucao.rotas[i][j]);
                    const Node& noC = nodeById(solucao.rotas[i][k]);
                    const Node& noD =
                        (k + 1 == solucao.rotas[i].size()) ? depot
                                                            : nodeById(solucao.rotas[i][k + 1]);

                    const double antes =
                        instance.distancia(noA, noB) + instance.distancia(noC, noD);
                    const double depois =
                        instance.distancia(noA, noC) + instance.distancia(noB, noD);

                    const double delta = depois - antes;

                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestJ = static_cast<int>(j);
                        bestK = static_cast<int>(k);
                    }
                }
            }

            if (bestJ != -1) {
                std::reverse(solucao.rotas[i].begin() + bestJ, solucao.rotas[i].begin() + bestK + 1);
                melhorou = true;
            }
        }
    }

    return solucao;
}

Solution randomOpt2(Solution solucao, const VRPInstance& instance) {
    std::mt19937& gen = rngGlobal();

    if (solucao.rotas.empty()) {
        return solucao;
    }

    std::uniform_int_distribution<> rota(0, static_cast<int>(solucao.rotas.size()) - 1);
    int r = rota(gen);

    if (solucao.rotas[r].size() < 2) {
        return solucao;
    }

    std::uniform_int_distribution<> verificacao(0, static_cast<int>(solucao.rotas[r].size()) - 1);
    int j = verificacao(gen);
    int k = verificacao(gen);

    while (j == k) {
        k = verificacao(gen);
    }

    if (j > k) {
        std::swap(j, k);
    }

    std::reverse(solucao.rotas[r].begin() + j, solucao.rotas[r].begin() + k + 1);

    return solucao;
}
