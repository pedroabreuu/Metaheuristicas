#include <random>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <climits>
#include <chrono>
#include <utility>
#include "metaheuristicas/BRKGA.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

struct Cromossomo {
    std::vector<double> keys;
    Solution solution;
    int fitness = std::numeric_limits<int>::max();
};

static Cromossomo gerarCromossomoAleatorio(const VRPInstance& instance, std::mt19937& gen) {
    std::uniform_real_distribution<> chaves(0.0, 1.0);
    Cromossomo cromossomo;

    cromossomo.keys.resize(instance.nodes.size() - 1);

    for (double& key : cromossomo.keys) {
        key = chaves(gen);
    }

    return cromossomo;
}

static Solution decoder(const Cromossomo& cromossomo, const VRPInstance& instance) {
    std::vector<std::pair<double, int>> pares;
    pares.reserve(cromossomo.keys.size());

    for (size_t i = 0; i < cromossomo.keys.size(); i++) {
        int cliente = instance.nodes[i + 1].id;
        pares.push_back({cromossomo.keys[i], cliente});
    }

    std::sort(pares.begin(), pares.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

    const int n = static_cast<int>(pares.size());
    std::vector<int> clientes(n);
    for (int i = 0; i < n; i++) clientes[i] = pares[static_cast<size_t>(i)].second;

    const Node& depot = instance.getDepot();
    std::vector<std::vector<double>> custo(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));

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
            custo[i][j] = custoAtual + instance.distancia(*anterior, depot);
        }
    }

    int maxRotas = (instance.num_trucks > 0) ? instance.num_trucks : n;
    constexpr double INF = 1e18;
    std::vector<std::vector<double>> dp(maxRotas + 1, std::vector<double>(n + 1, INF));
    std::vector<std::vector<int>> pred(maxRotas + 1, std::vector<int>(n + 1, -1));
    dp[0][0] = 0.0;

    for (int r = 1; r <= maxRotas; r++) {
        for (int j = 1; j <= n; j++) {
            for (int i = 0; i < j; i++) {
                if (!std::isfinite(custo[i][j - 1]) || dp[r - 1][i] >= INF) continue;
                double candidato = dp[r - 1][i] + custo[i][j - 1];
                if (candidato < dp[r][j]) {
                    dp[r][j] = candidato;
                    pred[r][j] = i;
                }
            }
        }
    }

    int melhorR = 1;
    for (int r = 2; r <= maxRotas; r++) {
        if (dp[r][n] < dp[melhorR][n]) melhorR = r;
    }

    Solution solucao;
    if (pred[melhorR][n] == -1) {
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

    int j = n;
    int r = melhorR;
    while (j > 0 && r > 0) {
        int i = pred[r][j];
        if (i < 0) break;
        std::vector<int> rota;
        for (int p = i; p < j; p++) rota.push_back(clientes[static_cast<size_t>(p)]);
        solucao.rotas.push_back(std::move(rota));
        j = i;
        r--;
    }
    std::reverse(solucao.rotas.begin(), solucao.rotas.end());
    return solucao;
}


static Cromossomo crossoverBiased(const Cromossomo& elite, const Cromossomo& naoElite, double probElite, std::mt19937& gen) {
    Cromossomo cromossomo;
    cromossomo.keys.resize(elite.keys.size());
    std::uniform_real_distribution<> distProbabilidade(0.0, 1.0);

    for (size_t i = 0; i < cromossomo.keys.size(); i++) {
        double probabilidade = distProbabilidade(gen);

        if (probabilidade > probElite) cromossomo.keys[i] = naoElite.keys[i];
        else cromossomo.keys[i] = elite.keys[i];
    }

    return cromossomo;
}

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

static void decodificaEAvalia(Cromossomo& c, const VRPInstance& instance) {
    c.solution = decoder(c, instance);

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

    c.solution.calculaCusto(instance);
    int k = 0;
    while (k < static_cast<int>(vizinhancas.size())) {
        limpaRotasVazias(c.solution);
        Solution candidata = vizinhancas[k](c.solution, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);
        if (candidata.custoTotal < c.solution.custoTotal) {
            c.solution = std::move(candidata);
            k = 0;
        } else {
            k++;
        }
    }

    limpaRotasVazias(c.solution);
    c.solution.calculaCusto(instance);
    c.fitness = c.solution.custoTotal;
}

Solution BRKGA(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int numElite, double mutantes, double probElite) {
    std::vector<Cromossomo> cromossomos;
    std::random_device rd;
    std::mt19937 gen(rd());
    int numMutantes = static_cast<int>(mutantes * tamanhoPopulacao);
    int numFilhos = tamanhoPopulacao - numElite - numMutantes;
    std::uniform_int_distribution<> eliteAleatorio(0, numElite - 1);
    std::uniform_int_distribution<> naoEliteAleatorio(numElite, tamanhoPopulacao - 1);

    for (int i = 0; i < tamanhoPopulacao; i++) {
        Cromossomo c = gerarCromossomoAleatorio(instance, gen);
        decodificaEAvalia(c, instance);
        cromossomos.push_back(c);
    }

    auto melhorGlobal = std::min_element(cromossomos.begin(), cromossomos.end(),
        [](const Cromossomo& a, const Cromossomo& b) {
            return a.fitness < b.fitness;
        });
    Cromossomo melhorCromossomo = *melhorGlobal;

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    for (int i = 0; i < numGeracoes; i++) {
        std::nth_element(cromossomos.begin(), cromossomos.begin() + numElite, cromossomos.end(),
            [](const Cromossomo& a, const Cromossomo& b) {
                return a.fitness < b.fitness;
            });

        std::vector<Cromossomo> novaGeracao(cromossomos.begin(), cromossomos.begin() + numElite);

        for (int j = 0; j < numMutantes; j++) {
            Cromossomo c = gerarCromossomoAleatorio(instance, gen);
            decodificaEAvalia(c, instance);
            novaGeracao.push_back(c);
        }

        for (int k = 0; k < numFilhos; k++) {
            int paiElite = eliteAleatorio(gen);
            int paiNaoElite = naoEliteAleatorio(gen);

            Cromossomo c = crossoverBiased(cromossomos[paiElite], cromossomos[paiNaoElite], probElite, gen);
            decodificaEAvalia(c, instance);
            novaGeracao.push_back(c);
        }

        cromossomos = novaGeracao;

        auto melhorAtual = std::min_element(cromossomos.begin(), cromossomos.end(),
            [](const Cromossomo& a, const Cromossomo& b) {
                return a.fitness < b.fitness;
            });

        if (melhorAtual->fitness < melhorCromossomo.fitness) {
            melhorCromossomo = *melhorAtual;
            tempoBest = std::chrono::steady_clock::now();
        }

        if (instance.optimal_value > 0 && melhorCromossomo.fitness == instance.optimal_value) break;

        if ((i + 1) % 100 == 0 || i == numGeracoes - 1) {
            std::cout << "Geracao " << i + 1
                    << " | Melhor geracao: " << melhorAtual->fitness
                    << " | Melhor global: " << melhorCromossomo.fitness << std::endl;
        }
    }

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    if (instance.optimal_value > 0 && melhorCromossomo.fitness == instance.optimal_value)
        std::cout << "BRKGA: Otimo encontrado em " << tempoMelhor << "s" << std::endl;
    else
        std::cout << "BRKGA: Melhor=" << melhorCromossomo.fitness << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

    return melhorCromossomo.solution;
}
