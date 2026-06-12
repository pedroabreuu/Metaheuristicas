#include "utils/rng.h"
#include <algorithm>
#include <random>
#include <vector>
#include "neighborhoods/orOpt.h"

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

Solution orOptIntra(Solution solucao, const VRPInstance& instance, int maxSegmento) {
    maxSegmento = std::max(1, maxSegmento);

    bool melhorou = true;
    while (melhorou) {
        melhorou = false;
        double melhorDelta = 0.0;
        int bestR = -1, bestIni = -1, bestTam = -1, bestPos = -1;

        for (size_t r = 0; r < solucao.rotas.size(); r++) {
            const auto& rota = solucao.rotas[r];
            const int n = static_cast<int>(rota.size());
            if (n < 3) continue;

            const double custoAntes = custoRota(rota, instance);
            for (int tam = 1; tam <= maxSegmento && tam < n; tam++) {
                for (int ini = 0; ini + tam <= n; ini++) {
                    std::vector<int> segmento(rota.begin() + ini, rota.begin() + ini + tam);
                    std::vector<int> base = rota;
                    base.erase(base.begin() + ini, base.begin() + ini + tam);

                    for (int pos = 0; pos <= static_cast<int>(base.size()); pos++) {
                        if (pos == ini) continue;
                        std::vector<int> candidata = base;
                        candidata.insert(candidata.begin() + pos, segmento.begin(), segmento.end());
                        double delta = custoRota(candidata, instance) - custoAntes;
                        if (delta < melhorDelta) {
                            melhorDelta = delta;
                            bestR = static_cast<int>(r);
                            bestIni = ini;
                            bestTam = tam;
                            bestPos = pos;
                        }
                    }
                }
            }
        }

        if (bestR != -1) {
            auto& rota = solucao.rotas[bestR];
            std::vector<int> segmento(rota.begin() + bestIni, rota.begin() + bestIni + bestTam);
            rota.erase(rota.begin() + bestIni, rota.begin() + bestIni + bestTam);
            rota.insert(rota.begin() + bestPos, segmento.begin(), segmento.end());
            melhorou = true;
        }
    }

    return solucao;
}

Solution orOptIntra2(Solution solucao, const VRPInstance& instance) {
    return orOptIntra(std::move(solucao), instance, 2);
}

Solution orOptIntra3(Solution solucao, const VRPInstance& instance) {
    return orOptIntra(std::move(solucao), instance, 3);
}

Solution orOptInter(Solution solucao, const VRPInstance& instance, int maxSegmento) {
    maxSegmento = std::max(1, maxSegmento);

    std::vector<int> cargas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); r++) {
        cargas[r] = cargaRota(solucao.rotas[r], instance);
    }

    bool melhorou = true;
    while (melhorou) {
        melhorou = false;
        double melhorDelta = 0.0;
        int bestOrigem = -1, bestDestino = -1, bestIni = -1, bestTam = -1, bestPos = -1;
        int bestDemanda = 0;

        for (size_t origem = 0; origem < solucao.rotas.size(); origem++) {
            const auto& rotaOrigem = solucao.rotas[origem];
            const int n = static_cast<int>(rotaOrigem.size());
            if (n == 0) continue;
            double custoOrigemAntes = custoRota(rotaOrigem, instance);

            for (int tam = 1; tam <= maxSegmento && tam <= n; tam++) {
                for (int ini = 0; ini + tam <= n; ini++) {
                    std::vector<int> segmento(rotaOrigem.begin() + ini, rotaOrigem.begin() + ini + tam);
                    int demandaSegmento = 0;
                    for (int cliente : segmento) demandaSegmento += instance.getNode(cliente).demanda;

                    std::vector<int> origemSemSegmento = rotaOrigem;
                    origemSemSegmento.erase(origemSemSegmento.begin() + ini, origemSemSegmento.begin() + ini + tam);
                    double custoOrigemDepois = origemSemSegmento.empty() ? 0.0 : custoRota(origemSemSegmento, instance);

                    for (size_t destino = 0; destino < solucao.rotas.size(); destino++) {
                        if (destino == origem) continue;
                        if (cargas[destino] + demandaSegmento > instance.capacity) continue;

                        const auto& rotaDestino = solucao.rotas[destino];
                        double custoDestinoAntes = custoRota(rotaDestino, instance);
                        for (int pos = 0; pos <= static_cast<int>(rotaDestino.size()); pos++) {
                            std::vector<int> destinoComSegmento = rotaDestino;
                            destinoComSegmento.insert(destinoComSegmento.begin() + pos, segmento.begin(), segmento.end());
                            double delta = (custoOrigemDepois + custoRota(destinoComSegmento, instance))
                                         - (custoOrigemAntes + custoDestinoAntes);
                            if (delta < melhorDelta) {
                                melhorDelta = delta;
                                bestOrigem = static_cast<int>(origem);
                                bestDestino = static_cast<int>(destino);
                                bestIni = ini;
                                bestTam = tam;
                                bestPos = pos;
                                bestDemanda = demandaSegmento;
                            }
                        }
                    }
                }
            }
        }

        if (bestOrigem != -1) {
            auto& origem = solucao.rotas[bestOrigem];
            std::vector<int> segmento(origem.begin() + bestIni, origem.begin() + bestIni + bestTam);
            origem.erase(origem.begin() + bestIni, origem.begin() + bestIni + bestTam);
            solucao.rotas[bestDestino].insert(solucao.rotas[bestDestino].begin() + bestPos, segmento.begin(), segmento.end());
            cargas[bestOrigem] -= bestDemanda;
            cargas[bestDestino] += bestDemanda;
            if (solucao.rotas[bestOrigem].empty()) {
                solucao.rotas.erase(solucao.rotas.begin() + bestOrigem);
                cargas.erase(cargas.begin() + bestOrigem);
            }
            melhorou = true;
        }
    }

    return solucao;
}

Solution orOptInter2(Solution solucao, const VRPInstance& instance) {
    return orOptInter(std::move(solucao), instance, 2);
}

Solution orOptInter3(Solution solucao, const VRPInstance& instance) {
    return orOptInter(std::move(solucao), instance, 3);
}

Solution randomOrOptInter(Solution solucao, const VRPInstance& instance, int maxSegmento) {
    std::mt19937& gen = rngGlobal();
    if (solucao.rotas.size() < 2) return solucao;
    maxSegmento = std::max(1, maxSegmento);

    std::vector<int> cargas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); r++) cargas[r] = cargaRota(solucao.rotas[r], instance);

    std::uniform_int_distribution<int> rotaDist(0, static_cast<int>(solucao.rotas.size()) - 1);
    for (int tentativa = 0; tentativa < 80; tentativa++) {
        int origem = rotaDist(gen);
        int destino = rotaDist(gen);
        if (origem == destino || solucao.rotas[origem].empty()) continue;

        int maxTam = std::min(maxSegmento, static_cast<int>(solucao.rotas[origem].size()));
        std::uniform_int_distribution<int> tamDist(1, maxTam);
        int tam = tamDist(gen);
        std::uniform_int_distribution<int> iniDist(0, static_cast<int>(solucao.rotas[origem].size()) - tam);
        int ini = iniDist(gen);

        int demanda = 0;
        for (int p = ini; p < ini + tam; p++) demanda += instance.getNode(solucao.rotas[origem][p]).demanda;
        if (cargas[destino] + demanda > instance.capacity) continue;

        std::vector<int> segmento(solucao.rotas[origem].begin() + ini, solucao.rotas[origem].begin() + ini + tam);
        std::uniform_int_distribution<int> posDist(0, static_cast<int>(solucao.rotas[destino].size()));
        int pos = posDist(gen);
        solucao.rotas[origem].erase(solucao.rotas[origem].begin() + ini, solucao.rotas[origem].begin() + ini + tam);
        solucao.rotas[destino].insert(solucao.rotas[destino].begin() + pos, segmento.begin(), segmento.end());
        if (solucao.rotas[origem].empty()) {
            solucao.rotas.erase(solucao.rotas.begin() + origem);
        }
        return solucao;
    }

    return solucao;
}

Solution randomOrOptInter3(Solution solucao, const VRPInstance& instance) {
    return randomOrOptInter(std::move(solucao), instance, 3);
}
