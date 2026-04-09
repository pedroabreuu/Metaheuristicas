#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include <chrono>
#include "metaheuristicas/BRKGA.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/relocate.h"

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
    int demandaAcumulada = 0;
    std::vector<int> rotaAtual;
    Solution solucao;

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

    for (const auto& [key, cliente] : pares) {
        int demanda = instance.getNode(cliente).demanda;

        if (demandaAcumulada + demanda <= instance.capacity) {
            rotaAtual.push_back(cliente);
            demandaAcumulada += demanda;
        } else {
            if (!rotaAtual.empty()) {
                solucao.rotas.push_back(rotaAtual);
            }
            rotaAtual.clear();
            rotaAtual.push_back(cliente);
            demandaAcumulada = demanda;
        }
    }

    if (!rotaAtual.empty()) {
        solucao.rotas.push_back(rotaAtual);
    }

    solucao.rotas.erase(
    std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
        [](const std::vector<int>& rota) { return rota.empty(); }),
    solucao.rotas.end());

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

    limpaRotasVazias(c.solution);
    c.solution = relocate(c.solution, instance);

    limpaRotasVazias(c.solution);
    c.solution = opt2(c.solution, instance);

    limpaRotasVazias(c.solution);
    c.solution = relocate(c.solution, instance);

    limpaRotasVazias(c.solution);
    c.solution = opt2(c.solution, instance);

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