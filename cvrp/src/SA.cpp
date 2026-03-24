#include <random>
#include <cmath>
#include "SA.h"
#include "2opt.h"
#include "relocate.h"

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
            corrente.calculaCusto(instance);

            delta = sl.custoTotal - corrente.custoTotal;

            if (delta <= 0) {
                corrente = sl;
            } else {
                double r = realdist(gen);
                if (r < std::exp(-delta / Temp)) {
                    corrente = sl;
                }
            }

            corrente.calculaCusto(instance);
            best.calculaCusto(instance);

            if (corrente.custoTotal < best.custoTotal) {
                best = corrente;
            }

            iterT++;
        }

        Temp *= alpha;
    }

    return best;
}