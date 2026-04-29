#include <vector>
#include <algorithm>
#include <limits>
#include <random>
#include "neighborhoods/swap.h"

static int cargaRota(const std::vector<int>& rota, const VRPInstance& instance) {
    int carga = 0;
    for (int clienteId : rota) {
        carga += instance.getNode(clienteId).demanda;
    }
    return carga;
}

static double custoRota(const std::vector<int>& rota, const VRPInstance& instance) {
    double custo = 0.0;
    const Node& depot = instance.getDepot();
    const Node* anterior = &depot;

    for (int clienteId : rota) {
        const Node& cliente = instance.getNode(clienteId);
        custo += instance.distancia(*anterior, cliente);
        anterior = &cliente;
    }

    custo += instance.distancia(*anterior, depot);
    return custo;
}

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

Solution swapInter(Solution solucao, const VRPInstance& instance) {
    std::vector<int> cargaRotas(solucao.rotas.size(), 0);
    std::vector<double> custoRotas(solucao.rotas.size(), 0.0);

    for (size_t r = 0; r < solucao.rotas.size(); r++) {
        cargaRotas[r] = cargaRota(solucao.rotas[r], instance);
        custoRotas[r] = custoRota(solucao.rotas[r], instance);
    }

    bool melhorou = true;

    while (melhorou) {
        melhorou = false;

        double bestDelta = 0.0;
        int bestR1 = -1;
        int bestR2 = -1;
        int bestI = -1;
        int bestJ = -1;

        for (size_t r1 = 0; r1 < solucao.rotas.size(); r1++) {
            if (solucao.rotas[r1].empty()) continue;

            for (size_t r2 = r1 + 1; r2 < solucao.rotas.size(); r2++) {
                if (solucao.rotas[r2].empty()) continue;

                for (size_t i = 0; i < solucao.rotas[r1].size(); i++) {
                    const int cliente1 = solucao.rotas[r1][i];
                    const int demanda1 = instance.getNode(cliente1).demanda;

                    for (size_t j = 0; j < solucao.rotas[r2].size(); j++) {
                        const int cliente2 = solucao.rotas[r2][j];
                        const int demanda2 = instance.getNode(cliente2).demanda;

                        const int novaCargaR1 = cargaRotas[r1] - demanda1 + demanda2;
                        const int novaCargaR2 = cargaRotas[r2] - demanda2 + demanda1;

                        if (novaCargaR1 > instance.capacity || novaCargaR2 > instance.capacity) {
                            continue;
                        }

                        std::vector<int> rota1Nova = solucao.rotas[r1];
                        std::vector<int> rota2Nova = solucao.rotas[r2];
                        std::swap(rota1Nova[i], rota2Nova[j]);

                        const double antes = custoRotas[r1] + custoRotas[r2];
                        const double depois = custoRota(rota1Nova, instance) + custoRota(rota2Nova, instance);
                        const double delta = depois - antes;

                        if (delta < bestDelta) {
                            bestDelta = delta;
                            bestR1 = static_cast<int>(r1);
                            bestR2 = static_cast<int>(r2);
                            bestI = static_cast<int>(i);
                            bestJ = static_cast<int>(j);
                        }
                    }
                }
            }
        }

        if (bestR1 != -1) {
            const int cliente1 = solucao.rotas[bestR1][bestI];
            const int cliente2 = solucao.rotas[bestR2][bestJ];

            std::swap(solucao.rotas[bestR1][bestI], solucao.rotas[bestR2][bestJ]);

            cargaRotas[bestR1] = cargaRotas[bestR1]
                - instance.getNode(cliente1).demanda
                + instance.getNode(cliente2).demanda;
            cargaRotas[bestR2] = cargaRotas[bestR2]
                - instance.getNode(cliente2).demanda
                + instance.getNode(cliente1).demanda;

            custoRotas[bestR1] = custoRota(solucao.rotas[bestR1], instance);
            custoRotas[bestR2] = custoRota(solucao.rotas[bestR2], instance);
            melhorou = true;
        }
    }

    return solucao;
}

Solution randomSwapIntra(Solution solucao, const VRPInstance& instance) {
    static std::mt19937 gen(std::random_device{}());
    (void) instance;

    if (solucao.rotas.empty()) {
        return solucao;
    }

    std::uniform_int_distribution<> rotaDist(0, static_cast<int>(solucao.rotas.size()) - 1);

    for (int tentativa = 0; tentativa < 50; tentativa++) {
        int r = rotaDist(gen);
        if (solucao.rotas[r].size() < 2) continue;

        std::uniform_int_distribution<> clienteDist(0, static_cast<int>(solucao.rotas[r].size()) - 1);
        int i = clienteDist(gen);
        int j = clienteDist(gen);

        while (j == i) {
            j = clienteDist(gen);
        }

        std::swap(solucao.rotas[r][i], solucao.rotas[r][j]);
        return solucao;
    }

    return solucao;
}

Solution randomSwapInter(Solution solucao, const VRPInstance& instance) {
    static std::mt19937 gen(std::random_device{}());

    if (solucao.rotas.size() < 2) {
        return solucao;
    }

    std::vector<int> cargaRotas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); r++) {
        cargaRotas[r] = cargaRota(solucao.rotas[r], instance);
    }

    std::uniform_int_distribution<> rotaDist(0, static_cast<int>(solucao.rotas.size()) - 1);

    for (int tentativa = 0; tentativa < 50; tentativa++) {
        int r1 = rotaDist(gen);
        int r2 = rotaDist(gen);

        while (r2 == r1) {
            r2 = rotaDist(gen);
        }

        if (solucao.rotas[r1].empty() || solucao.rotas[r2].empty()) {
            continue;
        }

        std::uniform_int_distribution<> clienteDist1(0, static_cast<int>(solucao.rotas[r1].size()) - 1);
        std::uniform_int_distribution<> clienteDist2(0, static_cast<int>(solucao.rotas[r2].size()) - 1);

        int i = clienteDist1(gen);
        int j = clienteDist2(gen);
        int cliente1 = solucao.rotas[r1][i];
        int cliente2 = solucao.rotas[r2][j];
        int demanda1 = instance.getNode(cliente1).demanda;
        int demanda2 = instance.getNode(cliente2).demanda;

        int novaCargaR1 = cargaRotas[r1] - demanda1 + demanda2;
        int novaCargaR2 = cargaRotas[r2] - demanda2 + demanda1;

        if (novaCargaR1 > instance.capacity || novaCargaR2 > instance.capacity) {
            continue;
        }

        std::swap(solucao.rotas[r1][i], solucao.rotas[r2][j]);
        return solucao;
    }

    return solucao;
}
