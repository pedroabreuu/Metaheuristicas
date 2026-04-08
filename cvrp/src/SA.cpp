#include <random>
#include <iostream>
#include <cmath>
#include <chrono>
#include "SA.h"
#include "relocate.h"
#include "2opt.h"

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double To, int SAmax, double alpha) {
    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;
    int iterT = 0;
    double Temp = To;
    double delta = 0.0;
    bool achouOtimo = false;
    
    Solution best = solucao;
    Solution corrente = solucao;
    Solution sl;
    
    best.calculaCusto(instance);
    corrente.calculaCusto(instance);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> movimento(0, 1);
    std::uniform_real_distribution<double> realdist(0.0, 1.0);
    
    while (Temp > 0.0001 && !achouOtimo) {
        iterT = 0;
        while (iterT < SAmax && !achouOtimo) {
            int movimentoAle = movimento(gen);
            
            if (movimentoAle == 0) {
                sl = randomRelocate(corrente, instance);
            } else {
                sl = randomOpt2(corrente, instance);
            }
            
            sl.calculaCusto(instance);
            
            delta = sl.custoTotal - corrente.custoTotal;
            
            if (delta <= 0) {
                corrente = sl;
                if (corrente.custoTotal < best.custoTotal) {
                    best = corrente;
                    tempoBest = std::chrono::steady_clock::now();
                    if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) { achouOtimo = true; }
                }
            } else {
                double r = realdist(gen);
                if (r < std::exp(-delta / Temp)) {
                    corrente = sl;
                }
            }
            
            iterT++;
        }
        Temp *= alpha;
        //std::cout << "Temp=" << Temp << " best=" << best.custoTotal << std::endl;
        //std::cout << "Temp=" << Temp << " corrente=" << corrente.custoTotal << " best=" << best.custoTotal << std::endl;
    }

    best = opt2(best, instance);
    best = relocate(best, instance);
    best.calculaCusto(instance);

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();

    if (best.custoTotal < corrente.custoTotal) {
        tempoBest = fim;
    }

    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value)
        std::cout << "SA: Otimo encontrado em " << tempoMelhor << "s" << std::endl;
    else
        std::cout << "SA: Melhor=" << best.custoTotal << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

    return best;
}