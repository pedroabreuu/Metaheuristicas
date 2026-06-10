#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include "utils/rkDecoder.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

namespace {

constexpr double INF = 1e18;

std::vector<std::vector<double>> custosSegmentos(const std::vector<int>& clientes,
                                                 const VRPInstance& instance) {
    const int n = static_cast<int>(clientes.size());
    const Node& depot = instance.getDepot();
    std::vector<std::vector<double>> custoSeg(static_cast<size_t>(n));

    for (int i = 0; i < n; i++) {
        int carga = 0;
        double custoAtual = 0.0;
        const Node* anterior = &depot;
        for (int j = i; j < n; j++) {
            const Node& cliente = instance.getNode(clientes[static_cast<size_t>(j)]);
            carga += cliente.demanda;
            if (carga > instance.capacity) break;
            custoAtual += instance.distancia(*anterior, cliente);
            anterior = &cliente;
            custoSeg[static_cast<size_t>(i)].push_back(custoAtual + instance.distancia(*anterior, depot));
        }
    }
    return custoSeg;
}

} // namespace

Solution decodeRandomKeys(const std::vector<double>& keys, const VRPInstance& instance) {
    std::vector<std::pair<double, int>> pares;
    pares.reserve(keys.size());

    for (size_t i = 0; i < keys.size(); i++) {
        int cliente = instance.nodes[i + 1].id;
        pares.push_back({keys[i], cliente});
    }

    std::sort(pares.begin(), pares.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

    const int n = static_cast<int>(pares.size());
    std::vector<int> clientes(static_cast<size_t>(n));
    for (int i = 0; i < n; i++) clientes[static_cast<size_t>(i)] = pares[static_cast<size_t>(i)].second;

    const auto custoSeg = custosSegmentos(clientes, instance);
    const int maxRotas = (instance.num_trucks > 0) ? instance.num_trucks : n;

    Solution solucao;

    std::vector<double> dp(static_cast<size_t>(n) + 1, INF);
    std::vector<int> pred(static_cast<size_t>(n) + 1, -1);
    dp[0] = 0.0;

    for (int i = 0; i < n; i++) {
        if (dp[static_cast<size_t>(i)] >= INF) continue;
        const auto& seg = custoSeg[static_cast<size_t>(i)];
        for (size_t k = 0; k < seg.size(); k++) {
            int j = i + static_cast<int>(k) + 1;
            double candidato = dp[static_cast<size_t>(i)] + seg[k];
            if (candidato < dp[static_cast<size_t>(j)]) {
                dp[static_cast<size_t>(j)] = candidato;
                pred[static_cast<size_t>(j)] = i;
            }
        }
    }

    if (pred[static_cast<size_t>(n)] != -1) {
        int numRotas = 0;
        for (int j = n; j > 0; j = pred[static_cast<size_t>(j)]) numRotas++;

        if (numRotas <= maxRotas) {
            for (int j = n; j > 0; j = pred[static_cast<size_t>(j)]) {
                int i = pred[static_cast<size_t>(j)];
                std::vector<int> rota(clientes.begin() + i, clientes.begin() + j);
                solucao.rotas.push_back(std::move(rota));
            }
            std::reverse(solucao.rotas.begin(), solucao.rotas.end());
            return solucao;
        }

        std::vector<std::vector<double>> dp2(static_cast<size_t>(maxRotas) + 1, std::vector<double>(static_cast<size_t>(n) + 1, INF));
        std::vector<std::vector<int>> pred2(static_cast<size_t>(maxRotas) + 1, std::vector<int>(static_cast<size_t>(n) + 1, -1));
        dp2[0][0] = 0.0;

        for (int r = 1; r <= maxRotas; r++) {
            for (int i = 0; i < n; i++) {
                if (dp2[static_cast<size_t>(r - 1)][static_cast<size_t>(i)] >= INF) continue;
                const auto& seg = custoSeg[static_cast<size_t>(i)];
                for (size_t k = 0; k < seg.size(); k++) {
                    int j = i + static_cast<int>(k) + 1;
                    double candidato = dp2[static_cast<size_t>(r - 1)][static_cast<size_t>(i)] + seg[k];
                    if (candidato < dp2[static_cast<size_t>(r)][static_cast<size_t>(j)]) {
                        dp2[static_cast<size_t>(r)][static_cast<size_t>(j)] = candidato;
                        pred2[static_cast<size_t>(r)][static_cast<size_t>(j)] = i;
                    }
                }
            }
        }

        int melhorR = 1;
        for (int r = 2; r <= maxRotas; r++) {
            if (dp2[static_cast<size_t>(r)][static_cast<size_t>(n)] < dp2[static_cast<size_t>(melhorR)][static_cast<size_t>(n)])
                melhorR = r;
        }

        if (pred2[static_cast<size_t>(melhorR)][static_cast<size_t>(n)] != -1) {
            int j = n;
            int r = melhorR;
            while (j > 0 && r > 0) {
                int i = pred2[static_cast<size_t>(r)][static_cast<size_t>(j)];
                if (i < 0) break;
                std::vector<int> rota(clientes.begin() + i, clientes.begin() + j);
                solucao.rotas.push_back(std::move(rota));
                j = i;
                r--;
            }
            std::reverse(solucao.rotas.begin(), solucao.rotas.end());
            return solucao;
        }
    }

    std::vector<int> rotaAtual;
    int carga = 0;
    for (int cliente : clientes) {
        int dem = instance.getNode(cliente).demanda;
        if (!rotaAtual.empty() && carga + dem > instance.capacity) {
            solucao.rotas.push_back(rotaAtual);
            rotaAtual.clear();
            carga = 0;
        }
        rotaAtual.push_back(cliente);
        carga += dem;
    }
    if (!rotaAtual.empty()) solucao.rotas.push_back(rotaAtual);
    return solucao;
}

void encodeRandomKeys(const Solution& solucao, const VRPInstance& instance, std::vector<double>& keys) {
    const double passo = 1.0 / (static_cast<double>(keys.size()) + 1.0);
    size_t pos = 0;
    for (const auto& rota : solucao.rotas) {
        for (int cliente : rota) {
            size_t idx = instance.indice(cliente);
            pos++;
            keys[idx - 1] = passo * static_cast<double>(pos);
        }
    }
}

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

Solution decodeSemBuscaLocal(const std::vector<double>& keys, const VRPInstance& instance) {
    Solution solucao = decodeRandomKeys(keys, instance);
    limpaRotasVazias(solucao);
    solucao.calculaCusto(instance);
    return solucao;
}

Solution decodeEBuscaLocal(const std::vector<double>& keys, const VRPInstance& instance) {
    Solution solucao = decodeRandomKeys(keys, instance);

    std::vector<Solution (*)(Solution, const VRPInstance&)> vizinhancas = {
        opt2,
        relocate,
        orOptIntra3,
        orOptInter3,
        swapIntra,
        swapInter,
        crossExchange,
        opt2Star
    };

    solucao.calculaCusto(instance);
    int k = 0;
    while (k < static_cast<int>(vizinhancas.size())) {
        limpaRotasVazias(solucao);
        Solution candidata = vizinhancas[static_cast<size_t>(k)](solucao, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);
        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = std::move(candidata);
            k = 0;
        } else {
            k++;
        }
    }

    limpaRotasVazias(solucao);
    solucao.calculaCusto(instance);
    return solucao;
}
