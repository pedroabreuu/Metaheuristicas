#include <iostream>
#include <chrono>
#include "parser.h"
#include "nearestNeighbor.h"
#include "CWSavings.h"
#include "2opt.h"
#include "relocate.h"
#include "SA.h"
#include "ga.h"

int main(int argc, char* argv[]) {
    std::string filepath = (argc > 1) ? argv[1] : "data/A-n32-k5.vrp";

    try {
        VRPInstance instance = parseVRP(filepath);
        std::cout << "Instância carregada.\n";
        std::cout << "Depot ID:     " << instance.depot_id   << "\n";
        std::cout << "Capacidade:   " << instance.capacity   << "\n";
        std::cout << "Nós:          " << instance.nodes.size() << "\n";
        std::cout << "Caminhões:    " << instance.num_trucks  << "\n";
        std::cout << "Otimo conhecido: " << instance.optimal_value << "\n";

        Solution solucaoNN = nearestN(instance);
        solucaoNN.calculaCusto(instance);
        std::cout << "========Nearest Neighbor========" << "\n";
        solucaoNN.imprime(instance);

        auto t0 = std::chrono::steady_clock::now();

        Solution solucaoCW = clarkeWright(instance);
        solucaoCW.calculaCusto(instance);
        std::cout << "========Clarke-Wright========" << "\n";
        solucaoCW.imprime(instance);

        Solution solucao2opt = opt2(solucaoCW, instance);
        solucao2opt.calculaCusto(instance);
        std::cout << "========2-opt========" << "\n";
        solucao2opt.imprime(instance);

        Solution solucaoRelocate = relocate(solucao2opt, instance);
        solucaoRelocate.calculaCusto(instance);
        std::cout << "========Relocate========" << "\n";
        solucaoRelocate.imprime(instance);

        auto t1 = std::chrono::steady_clock::now();
        double tempoCW = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "CW+2opt+Relocate: " << solucaoRelocate.custoTotal << " em " << tempoCW << "s" << std::endl;

        Solution solucaoSA = SimulatedAnnealing(solucaoRelocate, instance, 1000.0, 1000, 0.995);
        solucaoSA.calculaCusto(instance);
        std::cout << "========Simulated Annealing========" << "\n";
        solucaoSA.imprime(instance);

        std::cout << "========Algoritmo Genético========" << "\n";
        int tamanhoPopulacao, numGeracoes, tamanhoTorneio, elitismo;
        double probMutacao, probCrossover;
         
        std::cout << "Tamanho da populacao: ";
        std::cin >> tamanhoPopulacao;
         
        std::cout << "Numero de geracoes: ";
        std::cin >> numGeracoes;
         
        std::cout << "Tamanho do torneio: ";
        std::cin >> tamanhoTorneio;
         
        std::cout << "Elitismo (num. individuos): ";
        std::cin >> elitismo;
         
        std::cout << "Probabilidade de mutacao (0.0 a 1.0): ";
        std::cin >> probMutacao;
         
        std::cout << "Probabilidade de crossover (0.0 a 1.0): ";
        std::cin >> probCrossover;
         
        Solution solucaoGA = GeneticAlgorithm(instance, numGeracoes, tamanhoPopulacao,
                                               tamanhoTorneio, elitismo, probMutacao, probCrossover);
        solucaoGA.imprime(instance);
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
