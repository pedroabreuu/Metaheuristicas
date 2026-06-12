#include "utils/rng.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <random>
#include "neighborhoods/crossE.h"

Solution crossExchange(Solution solucao, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();

    auto nodeById = [&](int id) -> const Node& {
        return instance.getNode(id);
    };

    std::vector<int> cargaRotas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); ++r) {
        for (int id : solucao.rotas[r]) {
            cargaRotas[r] += nodeById(id).demanda;
        }
    }

    constexpr double EPS = 1e-9;
    const int maxSegmento = 3;

    bool melhorou = true;

    while (melhorou) {
        melhorou = false;
        double bestDelta = 0.0;

        int bestR1 = -1;
        int bestR2 = -1;
        int bestIniR1 = -1, bestFimR1 = -1;
        int bestIniR2 = -1, bestFimR2 = -1;
        int bestDemandaSeg1 = 0;
        int bestDemandaSeg2 = 0;

        for (size_t r1 = 0; r1 < solucao.rotas.size(); r1++) {
            for (size_t r2 = r1 + 1; r2 < solucao.rotas.size(); r2++) {
                const auto& rota1 = solucao.rotas[r1];
                const auto& rota2 = solucao.rotas[r2];

                if (rota1.empty() || rota2.empty()) continue;

                for (size_t iniR1 = 0; iniR1 < rota1.size(); iniR1++) {
                    for (int tam1 = 1; tam1 <= maxSegmento; tam1++) {
                        size_t fimR1 = iniR1 + tam1 - 1;
                        if (fimR1 >= rota1.size()) break;

                        int demandaSegmento1 = 0;
                        for (size_t p = iniR1; p <= fimR1; p++) {
                            demandaSegmento1 += nodeById(rota1[p]).demanda;
                        }

                        for (size_t iniR2 = 0; iniR2 < rota2.size(); iniR2++) {
                            for (int tam2 = 1; tam2 <= maxSegmento; tam2++) {
                                size_t fimR2 = iniR2 + tam2 - 1;
                                if (fimR2 >= rota2.size()) break;

                                int demandaSegmento2 = 0;
                                for (size_t p = iniR2; p <= fimR2; p++) {
                                    demandaSegmento2 += nodeById(rota2[p]).demanda;
                                }

                                int novaCargaR1 = cargaRotas[r1] - demandaSegmento1 + demandaSegmento2;
                                int novaCargaR2 = cargaRotas[r2] - demandaSegmento2 + demandaSegmento1;

                                if (novaCargaR1 > instance.capacity || novaCargaR2 > instance.capacity) {
                                    continue;
                                }

                                const Node& pred1 =
                                    (iniR1 == 0) ? depot : nodeById(rota1[iniR1 - 1]);
                                const Node& first1 = nodeById(rota1[iniR1]);
                                const Node& last1 = nodeById(rota1[fimR1]);
                                const Node& succ1 =
                                    (fimR1 + 1 >= rota1.size()) ? depot : nodeById(rota1[fimR1 + 1]);

                                const Node& pred2 =
                                    (iniR2 == 0) ? depot : nodeById(rota2[iniR2 - 1]);
                                const Node& first2 = nodeById(rota2[iniR2]);
                                const Node& last2 = nodeById(rota2[fimR2]);
                                const Node& succ2 =
                                    (fimR2 + 1 >= rota2.size()) ? depot : nodeById(rota2[fimR2 + 1]);

                                const double antes =
                                    instance.distancia(pred1, first1) +
                                    instance.distancia(last1, succ1) +
                                    instance.distancia(pred2, first2) +
                                    instance.distancia(last2, succ2);

                                const double depois =
                                    instance.distancia(pred1, first2) +
                                    instance.distancia(last2, succ1) +
                                    instance.distancia(pred2, first1) +
                                    instance.distancia(last1, succ2);

                                const double delta = depois - antes;

                                if (delta < bestDelta - EPS) {
                                    bestDelta = delta;
                                    bestR1 = static_cast<int>(r1);
                                    bestR2 = static_cast<int>(r2);
                                    bestIniR1 = static_cast<int>(iniR1);
                                    bestFimR1 = static_cast<int>(fimR1);
                                    bestIniR2 = static_cast<int>(iniR2);
                                    bestFimR2 = static_cast<int>(fimR2);
                                    bestDemandaSeg1 = demandaSegmento1;
                                    bestDemandaSeg2 = demandaSegmento2;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (bestR1 != -1) {
            auto& rota1 = solucao.rotas[bestR1];
            auto& rota2 = solucao.rotas[bestR2];

            std::vector<int> seg1(
                rota1.begin() + bestIniR1,
                rota1.begin() + bestFimR1 + 1
            );
            std::vector<int> seg2(
                rota2.begin() + bestIniR2,
                rota2.begin() + bestFimR2 + 1
            );

            rota1.erase(rota1.begin() + bestIniR1, rota1.begin() + bestFimR1 + 1);
            rota2.erase(rota2.begin() + bestIniR2, rota2.begin() + bestFimR2 + 1);

            rota1.insert(rota1.begin() + bestIniR1, seg2.begin(), seg2.end());
            rota2.insert(rota2.begin() + bestIniR2, seg1.begin(), seg1.end());

            cargaRotas[bestR1] = cargaRotas[bestR1] - bestDemandaSeg1 + bestDemandaSeg2;
            cargaRotas[bestR2] = cargaRotas[bestR2] - bestDemandaSeg2 + bestDemandaSeg1;

            melhorou = true;
        }
    }

    return solucao;
}

Solution randomCrossExchange(Solution solucao, const VRPInstance& instance) {
    std::mt19937& gen = rngGlobal();

    if (solucao.rotas.size() < 2) return solucao;

    std::uniform_int_distribution<> distRota(0, solucao.rotas.size() - 1);
    int r1 = distRota(gen);
    int r2 = distRota(gen);
    while (r2 == r1) r2 = distRota(gen);

    if (solucao.rotas[r1].empty() || solucao.rotas[r2].empty()) return solucao;

    std::uniform_int_distribution<> distI1(0, solucao.rotas[r1].size() - 1);
    std::uniform_int_distribution<> distI2(0, solucao.rotas[r2].size() - 1);

    int ini1 = distI1(gen);
    int fim1 = distI1(gen);
    if (ini1 > fim1) std::swap(ini1, fim1);

    int ini2 = distI2(gen);
    int fim2 = distI2(gen);
    if (ini2 > fim2) std::swap(ini2, fim2);

    std::vector<int> seg1(solucao.rotas[r1].begin() + ini1, solucao.rotas[r1].begin() + fim1 + 1);
    std::vector<int> seg2(solucao.rotas[r2].begin() + ini2, solucao.rotas[r2].begin() + fim2 + 1);

    auto cargaSegmento = [&](const std::vector<int>& seg) {
        int c = 0;
        for (int id : seg) c += instance.getNode(id).demanda;
        return c;
    };

    auto cargaTotal = [&](const std::vector<int>& rota) {
        int c = 0;
        for (int id : rota) c += instance.getNode(id).demanda;
        return c;
    };

    int novaCargaR1 = cargaTotal(solucao.rotas[r1]) - cargaSegmento(seg1) + cargaSegmento(seg2);
    int novaCargaR2 = cargaTotal(solucao.rotas[r2]) - cargaSegmento(seg2) + cargaSegmento(seg1);

    if (novaCargaR1 > instance.capacity || novaCargaR2 > instance.capacity) {
        return solucao;
    }

    solucao.rotas[r1].erase(solucao.rotas[r1].begin() + ini1, solucao.rotas[r1].begin() + fim1 + 1);
    solucao.rotas[r2].erase(solucao.rotas[r2].begin() + ini2, solucao.rotas[r2].begin() + fim2 + 1);

    solucao.rotas[r1].insert(solucao.rotas[r1].begin() + ini1, seg2.begin(), seg2.end());
    solucao.rotas[r2].insert(solucao.rotas[r2].begin() + ini2, seg1.begin(), seg1.end());

    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());

    return solucao;
}
