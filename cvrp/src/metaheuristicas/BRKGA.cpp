#include "utils/experimento.h"
#include "utils/rng.h"
#include <random>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <climits>
#include <chrono>
#include <utility>
#include "metaheuristicas/BRKGA.h"
#include "utils/rkDecoder.h"

struct Cromossomo {
    std::vector<double> keys;
    Solution solution;
    int fitness = std::numeric_limits<int>::max();
    bool lsAplicado = false;
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

static void decodificaEAvalia(Cromossomo& c, const VRPInstance& instance, bool comLS) {
    if (comLS) {
        c.solution = decodeEBuscaLocal(c.keys, instance);
        encodeRandomKeys(c.solution, instance, c.keys);
        c.lsAplicado = true;
    } else {
        c.solution = decodeSemBuscaLocal(c.keys, instance);
        c.lsAplicado = false;
    }
    c.fitness = c.solution.custoTotal;
}

Solution BRKGA(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int numElite, double mutantes, double probElite, bool lsApenasElite, double tempoLimite) {
    std::vector<Cromossomo> cromossomos;
    std::mt19937& gen = rngGlobal();
    int numMutantes = static_cast<int>(mutantes * tamanhoPopulacao);
    int numFilhos = tamanhoPopulacao - numElite - numMutantes;
    std::uniform_int_distribution<> eliteAleatorio(0, numElite - 1);
    std::uniform_int_distribution<> naoEliteAleatorio(numElite, tamanhoPopulacao - 1);

    const bool comLS = !lsApenasElite;

    auto aplicaLSElites = [&](std::vector<Cromossomo>& pop) {
        if (!lsApenasElite) return;
        for (int e = 0; e < numElite; e++) {
            Cromossomo& c = pop[static_cast<size_t>(e)];
            if (c.lsAplicado) continue;
            c.solution = decodeEBuscaLocal(c.keys, instance);
            encodeRandomKeys(c.solution, instance, c.keys);
            c.fitness = c.solution.custoTotal;
            c.lsAplicado = true;
        }
    };

    for (int i = 0; i < tamanhoPopulacao; i++) {
        Cromossomo c = gerarCromossomoAleatorio(instance, gen);
        decodificaEAvalia(c, instance, comLS);
        cromossomos.push_back(c);
    }

    std::nth_element(cromossomos.begin(), cromossomos.begin() + numElite, cromossomos.end(),
        [](const Cromossomo& a, const Cromossomo& b) { return a.fitness < b.fitness; });
    aplicaLSElites(cromossomos);

    auto melhorGlobal = std::min_element(cromossomos.begin(), cromossomos.end(),
        [](const Cromossomo& a, const Cromossomo& b) {
            return a.fitness < b.fitness;
        });
    Cromossomo melhorCromossomo = *melhorGlobal;
    expRegistraMelhora(melhorCromossomo.fitness);

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    for (int i = 0; i < numGeracoes; i++) {
        if (tempoLimite > 0.0 &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count() >= tempoLimite) {
            break;
        }
        std::nth_element(cromossomos.begin(), cromossomos.begin() + numElite, cromossomos.end(),
            [](const Cromossomo& a, const Cromossomo& b) {
                return a.fitness < b.fitness;
            });

        aplicaLSElites(cromossomos);

        std::vector<Cromossomo> novaGeracao(cromossomos.begin(), cromossomos.begin() + numElite);

        for (int j = 0; j < numMutantes; j++) {
            Cromossomo c = gerarCromossomoAleatorio(instance, gen);
            decodificaEAvalia(c, instance, comLS);
            novaGeracao.push_back(c);
        }

        for (int k = 0; k < numFilhos; k++) {
            int paiElite = eliteAleatorio(gen);
            int paiNaoElite = naoEliteAleatorio(gen);

            Cromossomo c = crossoverBiased(cromossomos[paiElite], cromossomos[paiNaoElite], probElite, gen);
            decodificaEAvalia(c, instance, comLS);
            novaGeracao.push_back(c);
        }

        cromossomos = novaGeracao;

        auto melhorAtual = std::min_element(cromossomos.begin(), cromossomos.end(),
            [](const Cromossomo& a, const Cromossomo& b) {
                return a.fitness < b.fitness;
            });

        if (melhorAtual->fitness < melhorCromossomo.fitness) {
            melhorCromossomo = *melhorAtual;
            expRegistraMelhora(melhorCromossomo.fitness);
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
