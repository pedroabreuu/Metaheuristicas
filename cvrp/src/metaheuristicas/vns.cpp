#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <utility>
#include "metaheuristicas/vns.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

using NeighborhoodFunc = Solution (*)(Solution, const VRPInstance&);

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

Solution VND(Solution solucao, const VRPInstance& instance) {
    limpaRotasVazias(solucao);
    std::vector<NeighborhoodFunc> vizinhancas = {opt2, relocate, swapIntra, swapInter, crossExchange, swapIntra};

    solucao.calculaCusto(instance);
    int k = 0;

    while (k < static_cast<int>(vizinhancas.size())) {
        limpaRotasVazias(solucao);
        Solution candidata = vizinhancas[k](solucao, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);

        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = candidata;
            k = 0;
        } else {
            k++;
        }
    }

    return solucao;
}

Solution perturbar(Solution solucao, const VRPInstance& instance, int k) {
    std::vector<NeighborhoodFunc> perturbacoes = {
        randomRelocate,
        randomOpt2,
        randomCrossExchange,
        randomSwapInter,
        randomSwapIntra
    };
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> perturbacao(0, perturbacoes.size() - 1);

    for (int i = 0; i < k; i++) {
        int indicePerturbacao = perturbacao(gen);
        solucao = perturbacoes[indicePerturbacao](std::move(solucao), instance);
        limpaRotasVazias(solucao);
    }

    return solucao;
}

Solution VNS(Solution solucao, const VRPInstance& instance, int kMax, double tempoLimite, double tempInicial, double alpha, int iterSemMelhoraMax) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dist(0.0, 1.0);

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    solucao.calculaCusto(instance);
    solucao = VND(solucao, instance);
    Solution best = solucao;
    int k = 1;
    double temperatura = tempInicial;
    int iterSemMelhora = 0;
    int iteracao = 0;

    std::cout << "VNS inicio | Custo: " << best.custoTotal << std::endl;

    if (best.custoTotal == instance.optimal_value) {
        std::cout << "VNS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
        return best;
    }

    while (true) {
        auto agora = std::chrono::steady_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        if (tempoDecorrido >= tempoLimite) break;

        Solution candidata = perturbar(solucao, instance, k);
        candidata = VND(candidata, instance);
        candidata.calculaCusto(instance);

        double delta = candidata.custoTotal - solucao.custoTotal;

        if (delta < 0) {
            solucao = candidata;
            k = 1;
            iterSemMelhora = 0;

            if (solucao.custoTotal < best.custoTotal) {
                best = solucao;
                tempoBest = std::chrono::steady_clock::now();
                std::cout << "VNS iter " << iteracao << " | Melhor: " << best.custoTotal << " em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;

                if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
                    std::cout << "VNS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
                    return best;
                }
            }
        } else if (temperatura > 0 && dist(gen) < std::exp(-delta / temperatura)) {
            solucao = candidata;
            k = 1;
            iterSemMelhora++;
        } else {
            k++;
            if (k > kMax) k = 1;
            iterSemMelhora++;
        }

        temperatura *= alpha;

        if (iterSemMelhora >= iterSemMelhoraMax) {
            if (dist(gen) < 0.3) {
                solucao = best;
            } else {
                solucao = perturbar(best, instance, kMax * 2);
                solucao = VND(solucao, instance);
            }
            k = 1;
            temperatura = tempInicial;
            iterSemMelhora = 0;
        }

        iteracao++;
    }

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    std::cout << "VNS: Melhor=" << best.custoTotal << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

    return best;
}
