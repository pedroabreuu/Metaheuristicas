#include "utils/experimento.h"
#include "utils/rng.h"
#include <random>
#include <vector>
#include <limits>
#include <chrono>
#include <iostream>
#include "metaheuristicas/pso.h"
#include "utils/rkDecoder.h"

struct Particula {
    std::vector<double> x; // posicao [0,1]
    std::vector<double> v;
    std::vector<double> pbest;
    int pbestFit = std::numeric_limits<int>::max();
    int estagnacao = 0;
};

static double clamp01(double valor) {
    if (valor < 0.0) return 0.0;
    if (valor > 1.0) return 1.0;
    return valor;
}

Solution PSO(const VRPInstance& instance, double tempoLimite,
             int numParticulas, double w, double c1, double c2, double vMax) {
    std::mt19937& gen = rngGlobal();
    std::uniform_real_distribution<> u01(0.0, 1.0);

    const size_t D = instance.nodes.size() - 1;
    std::vector<Particula> enxame(static_cast<size_t>(numParticulas));

    const int maxEstagnacao = 30;
    const int maxEstagnacaoGlobal = 200;

    auto avaliaGuia = [&](const std::vector<double>& x) -> Solution {
        return decodeEBuscaLocal(x, instance);
    };

    std::vector<double> gbest; // melhor pos
    int gbestGuiaFit = std::numeric_limits<int>::max(); 

    Solution melhorSol;
    int melhorFit = std::numeric_limits<int>::max();

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    auto registraMelhor = [&](const Solution& s) {
        if (s.custoTotal < melhorFit) {
            melhorFit = s.custoTotal;
            melhorSol = s;
            expRegistraMelhora(melhorFit);
            tempoBest = std::chrono::steady_clock::now();
        }
    };

    for (Particula& p : enxame) {
        p.x.resize(D);
        p.v.assign(D, 0.0);
        for (double& xi : p.x) xi = u01(gen);

        Solution s = avaliaGuia(p.x);
        encodeRandomKeys(s, instance, p.x);
        registraMelhor(s);
        p.pbest = p.x;
        p.pbestFit = s.custoTotal;

        if (s.custoTotal < gbestGuiaFit) {
            gbestGuiaFit = s.custoTotal;
            gbest = p.x;
        }
    }

    int iter = 0;
    int estagnacaoGlobal = 0;

    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count() < tempoLimite) {
        const int melhorFitAnterior = melhorFit;

        for (size_t pi = 0; pi < enxame.size(); pi++) {
            Particula& p = enxame[pi];

            if (p.estagnacao >= maxEstagnacao) {
                for (double& xi : p.x) xi = u01(gen);
                p.v.assign(D, 0.0);
                p.estagnacao = 0;
                p.pbestFit = std::numeric_limits<int>::max();
            } else {
                for (size_t d = 0; d < D; d++) {
                    double r1 = u01(gen);
                    double r2 = u01(gen);
                    p.v[d] = w * p.v[d]
                           + c1 * r1 * (p.pbest[d] - p.x[d])
                           + c2 * r2 * (gbest[d]   - p.x[d]);
                    if (p.v[d] >  vMax) p.v[d] =  vMax;
                    if (p.v[d] < -vMax) p.v[d] = -vMax;
                    p.x[d] = clamp01(p.x[d] + p.v[d]);
                }
            }

            Solution s = avaliaGuia(p.x);
            encodeRandomKeys(s, instance, p.x);
            registraMelhor(s);

            if (s.custoTotal < p.pbestFit) {
                p.pbestFit = s.custoTotal;
                p.pbest = p.x;
                p.estagnacao = 0;
            } else {
                p.estagnacao++;
            }
            if (s.custoTotal < gbestGuiaFit) {
                gbestGuiaFit = s.custoTotal;
                gbest = p.x;
            }
        }

        if (instance.optimal_value > 0 && melhorFit == instance.optimal_value) break;

        if (melhorFit < melhorFitAnterior) {
            estagnacaoGlobal = 0;
        } else if (++estagnacaoGlobal >= maxEstagnacaoGlobal) {
            estagnacaoGlobal = 0;
            gbestGuiaFit = std::numeric_limits<int>::max();
            for (Particula& p : enxame) {
                for (double& xi : p.x) xi = u01(gen);
                p.v.assign(D, 0.0);
                p.estagnacao = 0;
                Solution s = avaliaGuia(p.x);
                encodeRandomKeys(s, instance, p.x);
                registraMelhor(s);
                p.pbest = p.x;
                p.pbestFit = s.custoTotal;
                if (s.custoTotal < gbestGuiaFit) {
                    gbestGuiaFit = s.custoTotal;
                    gbest = p.x;
                }
            }
        }

        if (++iter % 50 == 0) {
            std::cout << "Iter " << iter << " | melhor: " << melhorFit << std::endl;
        }
    }

    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();
    std::cout << "PSO: Melhor=" << melhorFit << " em " << tempoMelhor << "s" << std::endl;

    return melhorSol;
}
