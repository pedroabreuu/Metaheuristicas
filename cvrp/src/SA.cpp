#include <random>
#include <iostream>
#include <cmath>
#include "SA.h"
#include "relocate.h"
#include "2opt.h"

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double To, int SAmax, double alpha) {
    int iterT = 0;
    double Temp = To;
    double delta = 0.0;
    
    Solution best = solucao;
    Solution corrente = solucao;
    Solution sl;
    
    best.calculaCusto(instance);
    corrente.calculaCusto(instance);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> movimento(0, 1);
    std::uniform_real_distribution<double> realdist(0.0, 1.0);
    
    while (Temp > 0.0001) {
        iterT = 0;
        while (iterT < SAmax) {
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
        std::cout << "Temp=" << Temp << " corrente=" << corrente.custoTotal << " best=" << best.custoTotal << std::endl;
    }

    //best = opt2(best, instance);
    //best = relocate(best, instance);
    return best;
}