#include "utils/rng.h"
#include <algorithm>
#include <random>
#include <vector>
#include "neighborhoods/2optStar.h"

static int cargaRota(const std::vector<int>& rota, const VRPInstance& instance) {
    int carga = 0;
    for (int cliente : rota) carga += instance.getNode(cliente).demanda;
    return carga;
}

static double custoRota(const std::vector<int>& rota, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();
    const Node* anterior = &depot;
    double custo = 0.0;
    for (int cliente : rota) {
        const Node& atual = instance.getNode(cliente);
        custo += instance.distancia(*anterior, atual);
        anterior = &atual;
    }
    custo += instance.distancia(*anterior, depot);
    return custo;
}

Solution opt2Star(Solution solucao, const VRPInstance& instance) {
    std::vector<int> cargas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); r++) cargas[r] = cargaRota(solucao.rotas[r], instance);

    bool melhorou = true;
    while (melhorou) {
        melhorou = false;
        double melhorDelta = 0.0;
        int bestR1 = -1, bestR2 = -1, bestI = -1, bestJ = -1;
        int bestCargaR1 = 0, bestCargaR2 = 0;

        for (size_t r1 = 0; r1 < solucao.rotas.size(); r1++) {
            for (size_t r2 = r1 + 1; r2 < solucao.rotas.size(); r2++) {
                const auto& rota1 = solucao.rotas[r1];
                const auto& rota2 = solucao.rotas[r2];
                if (rota1.empty() || rota2.empty()) continue;

                double custoAntes = custoRota(rota1, instance) + custoRota(rota2, instance);
                for (int i = 0; i <= static_cast<int>(rota1.size()); i++) {
                    for (int j = 0; j <= static_cast<int>(rota2.size()); j++) {
                        std::vector<int> novaR1;
                        std::vector<int> novaR2;
                        novaR1.reserve(i + rota2.size() - j);
                        novaR2.reserve(j + rota1.size() - i);

                        novaR1.insert(novaR1.end(), rota1.begin(), rota1.begin() + i);
                        novaR1.insert(novaR1.end(), rota2.begin() + j, rota2.end());
                        novaR2.insert(novaR2.end(), rota2.begin(), rota2.begin() + j);
                        novaR2.insert(novaR2.end(), rota1.begin() + i, rota1.end());

                        if (novaR1.empty() || novaR2.empty()) continue;
                        int carga1 = cargaRota(novaR1, instance);
                        int carga2 = cargaRota(novaR2, instance);
                        if (carga1 > instance.capacity || carga2 > instance.capacity) continue;

                        double delta = custoRota(novaR1, instance) + custoRota(novaR2, instance) - custoAntes;
                        if (delta < melhorDelta) {
                            melhorDelta = delta;
                            bestR1 = static_cast<int>(r1);
                            bestR2 = static_cast<int>(r2);
                            bestI = i;
                            bestJ = j;
                            bestCargaR1 = carga1;
                            bestCargaR2 = carga2;
                        }
                    }
                }
            }
        }

        if (bestR1 != -1) {
            auto rota1 = solucao.rotas[bestR1];
            auto rota2 = solucao.rotas[bestR2];
            std::vector<int> novaR1;
            std::vector<int> novaR2;
            novaR1.insert(novaR1.end(), rota1.begin(), rota1.begin() + bestI);
            novaR1.insert(novaR1.end(), rota2.begin() + bestJ, rota2.end());
            novaR2.insert(novaR2.end(), rota2.begin(), rota2.begin() + bestJ);
            novaR2.insert(novaR2.end(), rota1.begin() + bestI, rota1.end());
            solucao.rotas[bestR1] = std::move(novaR1);
            solucao.rotas[bestR2] = std::move(novaR2);
            cargas[bestR1] = bestCargaR1;
            cargas[bestR2] = bestCargaR2;
            melhorou = true;
        }
    }

    return solucao;
}

Solution randomOpt2Star(Solution solucao, const VRPInstance& instance) {
    std::mt19937& gen = rngGlobal();
    if (solucao.rotas.size() < 2) return solucao;

    std::uniform_int_distribution<int> rotaDist(0, static_cast<int>(solucao.rotas.size()) - 1);
    for (int tentativa = 0; tentativa < 80; tentativa++) {
        int r1 = rotaDist(gen);
        int r2 = rotaDist(gen);
        if (r1 == r2 || solucao.rotas[r1].empty() || solucao.rotas[r2].empty()) continue;

        std::uniform_int_distribution<int> corte1(0, static_cast<int>(solucao.rotas[r1].size()));
        std::uniform_int_distribution<int> corte2(0, static_cast<int>(solucao.rotas[r2].size()));
        int i = corte1(gen);
        int j = corte2(gen);

        std::vector<int> novaR1;
        std::vector<int> novaR2;
        novaR1.insert(novaR1.end(), solucao.rotas[r1].begin(), solucao.rotas[r1].begin() + i);
        novaR1.insert(novaR1.end(), solucao.rotas[r2].begin() + j, solucao.rotas[r2].end());
        novaR2.insert(novaR2.end(), solucao.rotas[r2].begin(), solucao.rotas[r2].begin() + j);
        novaR2.insert(novaR2.end(), solucao.rotas[r1].begin() + i, solucao.rotas[r1].end());

        if (novaR1.empty() || novaR2.empty()) continue;
        if (cargaRota(novaR1, instance) > instance.capacity || cargaRota(novaR2, instance) > instance.capacity) continue;

        solucao.rotas[r1] = std::move(novaR1);
        solucao.rotas[r2] = std::move(novaR2);
        return solucao;
    }

    return solucao;
}
