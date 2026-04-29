#include <random>
#include <iostream>
#include <cmath>
#include <chrono>
#include "metaheuristicas/SA.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double To, int SAmax, double alpha) {
    auto inicio = std::chrono::steady_clock::now();
    int maxMelhoraTemp = 100;
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
    std::uniform_real_distribution<double> realdist(0.0, 1.0);

    double pesoRelocate = 1.0;
    double peso2Opt = 1.0;
    double pesoCross = 1.0;

    int semMelhoraBest = 0;
    int semMelhoraTemp = 0;
    const int limiteSemMelhoraBest = std::max(50, SAmax / 2);
    const int limiteSemMelhoraTemp = 8;

    auto intensificaParcial = [&](Solution s) {
        int escolha = std::uniform_int_distribution<>(0, 1)(gen);
        if (escolha == 0) {
            s = opt2(s, instance);
        } else {
            s = relocate(s, instance);
        }
        s.calculaCusto(instance);
        return s;
    };

    auto intensificaForte = [&](Solution s) {
        bool melhorou = true;
        int passos = 0;
        const int maxPassos = 2;

        while (melhorou && passos < maxPassos) {
            melhorou = false;

            Solution s1 = opt2(s, instance);
            if (s1.custoTotal < s.custoTotal) {
                s = s1;
                melhorou = true;
            }

            Solution s2 = relocate(s, instance);
            if (s2.custoTotal < s.custoTotal) {
                s = s2;
                melhorou = true;
            }

            Solution s3 = swapIntra(s, instance);
            if (s3.custoTotal < s.custoTotal) {
                s = s3;
                melhorou = true;
            }

            Solution s4 = crossExchange(s, instance);
            if (s4.custoTotal < s.custoTotal) {
                s = s4;
                melhorou = true;
            }

            passos++;
        }

        s.calculaCusto(instance);
        return s;
    };

    auto escolheMovimento = [&]() {
        double somaPesos = pesoRelocate + peso2Opt + pesoCross;
        double r = realdist(gen) * somaPesos;

        if (r < pesoRelocate) return 0;
        if (r < pesoRelocate + peso2Opt) return 1;
        return 2;
    };

    while (Temp > 0.0001 && !achouOtimo) {
        iterT = 0;
        bool melhorouNaTemperatura = false;

        while (iterT < SAmax && !achouOtimo) {
            int movimentoAle = escolheMovimento();
            
            if (movimentoAle == 0) {
                sl = randomRelocate(corrente, instance);
            } else if (movimentoAle == 1) {
                sl = randomOpt2(corrente, instance);
            } else {
                sl = randomCrossExchange(corrente, instance);
            }
            
            sl.calculaCusto(instance);
            delta = sl.custoTotal - corrente.custoTotal;
            
            if (delta <= 0) {
                corrente = sl;

                Solution correnteIntensificada = intensificaParcial(corrente);
                if (correnteIntensificada.custoTotal < corrente.custoTotal) {
                    corrente = correnteIntensificada;
                }

                if (movimentoAle == 0) pesoRelocate += 0.15;
                else if (movimentoAle == 1) peso2Opt += 0.15;
                else pesoCross += 0.15;

                if (corrente.custoTotal < best.custoTotal) {
                    best = corrente;

                    // itensificacao forte quando melhora best
                    best = intensificaForte(best);

                    if (best.custoTotal < corrente.custoTotal) {
                        corrente = best;
                    }

                    tempoBest = std::chrono::steady_clock::now();
                    melhorouNaTemperatura = true;
                    semMelhoraBest = 0;

                    if (movimentoAle == 0) pesoRelocate += 0.35;
                    else if (movimentoAle == 1) peso2Opt += 0.35;
                    else pesoCross += 0.35;

                    if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
                        achouOtimo = true;
                    }
                } else {
                    semMelhoraBest++;
                }

            } else {
                double r = realdist(gen);
                if (r < std::exp(-delta / Temp)) {
                    corrente = sl;

                    // recompensa se o movimento ao menos foi aceito
                    if (movimentoAle == 0) pesoRelocate += 0.03;
                    else if (movimentoAle == 1) peso2Opt += 0.03;
                    else pesoCross += 0.03;
                } else {
                    // penaliza se o movimento foi rejeitado
                    if (movimentoAle == 0) pesoRelocate = std::max(0.20, pesoRelocate - 0.02);
                    else if (movimentoAle == 1) peso2Opt = std::max(0.20, peso2Opt - 0.02);
                    else pesoCross = std::max(0.20, pesoCross - 0.02);

                    semMelhoraBest++;
                }
            }

            // normalizar para evitar pesos com vlaores altos
            double somaPesos = pesoRelocate + peso2Opt + pesoCross;
            if (somaPesos > 30.0) {
                pesoRelocate /= somaPesos;
                peso2Opt /= somaPesos;
                pesoCross /= somaPesos;

                pesoRelocate *= 3.0;
                peso2Opt *= 3.0;
                pesoCross *= 3.0;
            }
            
            // controle de estagnacao
            if (semMelhoraBest >= limiteSemMelhoraBest) {
                corrente = best;
                corrente = intensificaParcial(corrente);
                semMelhoraBest = 0;
            }
            
            iterT++;
        }

        if (melhorouNaTemperatura) {
            semMelhoraTemp = 0;
        } else {
            semMelhoraTemp++;
        }

        if (semMelhoraTemp >= limiteSemMelhoraTemp && maxMelhoraTemp > 0) {
            Temp = std::max(Temp, To * 0.05);
            corrente = best;
            corrente = intensificaParcial(corrente);
            semMelhoraTemp = 0;
            maxMelhoraTemp--;
        }

        if (maxMelhoraTemp <= 0) {
            corrente = randomOpt2(corrente, instance);
            corrente = randomRelocate(corrente, instance);
            corrente = randomCrossExchange(corrente, instance);
        }

        Temp *= alpha;
        std::cout << "Temp=" << Temp << " corrente=" << corrente.custoTotal << " best=" << best.custoTotal << std::endl;
    }

    bool melhorou = true;
    while (melhorou) {
        melhorou = false;

        Solution s1 = opt2(best, instance);
        if (s1.custoTotal < best.custoTotal) { best = s1; melhorou = true; continue; }

        Solution s2 = relocate(best, instance);
        if (s2.custoTotal < best.custoTotal) { best = s2; melhorou = true; continue; }

        Solution s3 = swapIntra(best, instance);
        if (s3.custoTotal < best.custoTotal) { best = s3; melhorou = true; continue; }

        Solution s4 = crossExchange(best, instance);
        if (s4.custoTotal < best.custoTotal) { best = s4; melhorou = true; continue; }

        Solution s5 = swapIntra(best, instance);
        if (s5.custoTotal < best.custoTotal) { best = s5; melhorou = true; continue; }
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