#include "utils/experimento.h"
#include "utils/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <utility>
#include <vector>
#include "metaheuristicas/SA.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

using NeighborhoodFunc = Solution (*)(Solution, const VRPInstance&);

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

static Solution buscaLocalCompleta(Solution solucao, const VRPInstance& instance) {
    std::vector<NeighborhoodFunc> vizinhancas = {
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
        Solution candidata = vizinhancas[k](solucao, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);
        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = std::move(candidata);
            k = 0;
        } else {
            k++;
        }
    }

    return solucao;
}

static Solution movimentoAleatorio(Solution solucao, const VRPInstance& instance, int movimento) {
    switch (movimento) {
        case 0: return randomRelocate(std::move(solucao), instance);
        case 1: return randomOpt2(std::move(solucao), instance);
        case 2: return randomCrossExchange(std::move(solucao), instance);
        case 3: return randomSwapIntra(std::move(solucao), instance);
        case 4: return randomSwapInter(std::move(solucao), instance);
        case 5: return randomOrOptInter(std::move(solucao), instance, 3);
        default: return randomOpt2Star(std::move(solucao), instance);
    }
}

static double estimaTemperaturaInicial(const Solution& inicial, const VRPInstance& instance, std::mt19937& gen) {
    Solution base = inicial;
    base.calculaCusto(instance);
    std::vector<double> deltas;
    deltas.reserve(200);

    std::uniform_int_distribution<int> movDist(0, 6);
    for (int i = 0; i < 250; i++) {
        Solution candidata = movimentoAleatorio(base, instance, movDist(gen));
        candidata.calculaCusto(instance);
        double delta = static_cast<double>(candidata.custoTotal - base.custoTotal);
        if (delta > 0.0) deltas.push_back(delta);
    }

    if (deltas.empty()) {
        return std::max(1.0, 0.05 * static_cast<double>(base.custoTotal));
    }

    double media = 0.0;
    for (double d : deltas) media += d;
    media /= static_cast<double>(deltas.size());
    return std::max(1.0, -media / std::log(0.80));
}

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double tempoLimiteSegundos, int SAmax, double alpha) {
    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    std::mt19937& gen = rngGlobal();
    std::uniform_real_distribution<double> realdist(0.0, 1.0);
    std::uniform_int_distribution<int> movDist(0, 6);

    Solution corrente = buscaLocalCompleta(std::move(solucao), instance);
    corrente.calculaCusto(instance);
    Solution best = corrente;
    expRegistraMelhora(best.custoTotal);

    double tempInicial = estimaTemperaturaInicial(corrente, instance, gen);
    double tempMin = std::max(0.01, tempInicial * 0.001);
    double temp = tempInicial;

    std::vector<double> pesos(7, 1.0);
    int iterSemMelhora = 0;
    int iteracao = 0;

    std::cout << "SA inicio | Custo: " << best.custoTotal
              << " | Tempo limite: " << tempoLimiteSegundos
              << "s | Temp inicial estimada: " << tempInicial << std::endl;

    if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
        std::cout << "SA: Otimo encontrado em 0s" << std::endl;
        return best;
    }

    while (true) {
        auto agora = std::chrono::steady_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        if (tempoDecorrido >= tempoLimiteSegundos) break;

        int aceitos = 0;
        int testados = 0;
        bool melhorouNaTemperatura = false;

        for (int i = 0; i < SAmax; i++) {
            agora = std::chrono::steady_clock::now();
            tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
            if (tempoDecorrido >= tempoLimiteSegundos) break;

            std::discrete_distribution<int> escolhaMovimento(pesos.begin(), pesos.end());
            int movimento = escolhaMovimento(gen);
            Solution candidata = movimentoAleatorio(corrente, instance, movimento);
            limpaRotasVazias(candidata);
            candidata.calculaCusto(instance);

            int delta = candidata.custoTotal - corrente.custoTotal;
            testados++;
            if (delta <= 0) {
                Solution polida = buscaLocalCompleta(candidata, instance);
                if (polida.custoTotal < candidata.custoTotal) {
                    candidata = std::move(polida);
                    delta = candidata.custoTotal - corrente.custoTotal;
                }
            }

            if (delta <= 0 || realdist(gen) < std::exp(-static_cast<double>(delta) / temp)) {
                corrente = std::move(candidata);
                aceitos++;
                pesos[static_cast<size_t>(movimento)] += (delta <= 0) ? 0.08 : 0.02;

                if (corrente.custoTotal < best.custoTotal) {
                    if (corrente.custoTotal < best.custoTotal) {
                        best = corrente;
                        expRegistraMelhora(best.custoTotal);
                        tempoBest = std::chrono::steady_clock::now();
                        melhorouNaTemperatura = true;
                        iterSemMelhora = 0;

                        std::cout << "SA iter " << iteracao
                                  << " | Melhor: " << best.custoTotal
                                  << " em "
                                  << std::chrono::duration<double>(tempoBest - inicio).count()
                                  << "s" << std::endl;

                        if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
                            std::cout << "SA: Otimo encontrado em "
                                      << std::chrono::duration<double>(tempoBest - inicio).count()
                                      << "s" << std::endl;
                            return best;
                        }
                    }
                }
            } else {
                pesos[static_cast<size_t>(movimento)] = std::max(0.20, pesos[static_cast<size_t>(movimento)] - 0.01);
            }
            iteracao++;
        }

        if (!melhorouNaTemperatura) iterSemMelhora++;

        double taxaAceitacao = (testados > 0) ? static_cast<double>(aceitos) / static_cast<double>(testados) : 0.0;
        if (iterSemMelhora >= 6 || taxaAceitacao < 0.02) {
            temp = std::max(temp, tempInicial * 0.25);
            corrente = best;
            corrente = movimentoAleatorio(std::move(corrente), instance, movDist(gen));
            corrente = movimentoAleatorio(std::move(corrente), instance, movDist(gen));
            corrente.calculaCusto(instance);
            iterSemMelhora = 0;
        } else {
            temp = std::max(tempMin, temp * alpha);
        }

        double somaPesos = 0.0;
        for (double peso : pesos) somaPesos += peso;
        if (somaPesos > 40.0) {
            for (double& peso : pesos) peso = std::max(0.20, peso / somaPesos * static_cast<double>(pesos.size()));
        }
    }

    {
        Solution polidaFinal = buscaLocalCompleta(best, instance);
        if (polidaFinal.custoTotal < best.custoTotal) {
            best = std::move(polidaFinal);
            expRegistraMelhora(best.custoTotal);
        }
    }

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value)
        std::cout << "SA: Otimo encontrado em " << tempoMelhor << "s" << std::endl;
    else
        std::cout << "SA: Melhor=" << best.custoTotal << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

    return best;
}
