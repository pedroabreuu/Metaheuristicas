#include <iostream>
#include <chrono>
#include "utils/parser.h"
#include "utils/nearestNeighbor.h"
#include "utils/CWSavings.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"
#include "metaheuristicas/SA.h"
#include "metaheuristicas/ga.h"
#include "metaheuristicas/BRKGA.h"
#include "metaheuristicas/vns.h"

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

        // Solution solucaoNN = nearestN(instance);
        // solucaoNN.calculaCusto(instance);
        // std::cout << "========Nearest Neighbor========" << "\n";
        // solucaoNN.imprime(instance);

        auto t0 = std::chrono::steady_clock::now();

        Solution solucaoCW = clarkeWright(instance);
        solucaoCW.calculaCusto(instance);
        //std::cout << "========Clarke-Wright========" << "\n";
        //solucaoCW.imprime(instance);

        Solution solucao2opt = opt2(solucaoCW, instance);
        solucao2opt.calculaCusto(instance);
        // std::cout << "========2-opt========" << "\n";
        // solucao2opt.imprime(instance);

        Solution solucaoRelocate = relocate(solucao2opt, instance);
        solucaoRelocate.calculaCusto(instance);
        // std::cout << "========Relocate========" << "\n";
        // solucaoRelocate.imprime(instance);

        Solution solucaoSWAP = swapIntra(solucaoRelocate, instance);
        solucaoSWAP.calculaCusto(instance);
        // std::cout << "========Swap Intra========" << "\n";
        // solucaoSWAP.imprime(instance);

        Solution solucaoCE = crossExchange(solucaoSWAP, instance);
        solucaoCE.calculaCusto(instance);
        // std::cout << "========Cross-Exchange========" << "\n";
        // solucaoCE.imprime(instance);

        solucaoSWAP = swapIntra(solucaoCE, instance);
        solucaoSWAP.calculaCusto(instance);
        // std::cout << "========Swap Intra 2========" << "\n";
        // solucaoSWAP.imprime(instance);

        // auto t1 = std::chrono::steady_clock::now();
        // double tempoCW = std::chrono::duration<double>(t1 - t0).count();
        // std::cout << "CW+2opt+Relocate+Swap+CrossExchange: "
        //           << solucaoSWAP.custoTotal << " em " << tempoCW << "s" << std::endl;

        // Solution solucaoSA = SimulatedAnnealing(solucaoCW, instance, 1000.0, 2000, 0.995);
        // solucaoSA.calculaCusto(instance);
        // std::cout << "========Simulated Annealing========" << "\n";
        // solucaoSA.imprime(instance);

        // std::cout << "========Algoritmo Genético========" << "\n";
        // int tamanhoPopulacao, numGeracoes, tamanhoTorneio, elitismo;
        // double probMutacao, probCrossover;
         
        // std::cout << "Tamanho da populacao: ";
        // std::cin >> tamanhoPopulacao;
         
        // std::cout << "Numero de geracoes: ";
        // std::cin >> numGeracoes;
         
        // std::cout << "Tamanho do torneio: ";
        // std::cin >> tamanhoTorneio;
         
        // std::cout << "Elitismo (num. individuos): ";
        // std::cin >> elitismo;
         
        // std::cout << "Probabilidade de mutacao (0.0 a 1.0): ";
        // std::cin >> probMutacao;
         
        // std::cout << "Probabilidade de crossover (0.0 a 1.0): ";
        // std::cin >> probCrossover;
         
        // Solution solucaoGA = GeneticAlgorithm(instance, 5000, 150,
        //                                        3, 2, 0.2, 0.75);
        // solucaoGA.imprime(instance);

        // std::cout << "========BRKGA========" << "\n";
        // int tamanhoPopulacaoBRKGA, numGeracoesBRKGA, numElite;
        // double mutantes, probElite;

        // std::cout << "Tamanho da populacao: ";
        // std::cin >> tamanhoPopulacaoBRKGA;
         
        // std::cout << "Numero de geracoes: ";
        // std::cin >> numGeracoesBRKGA;
         
        // std::cout << "Elitismo (num. individuos): ";
        // std::cin >> numElite;
         
        // std::cout << "Quantidade de mutantes %: ";
        // std::cin >> mutantes;
         
        // std::cout << "Probabilidade de selecionar elites: ";
        // std::cin >> probElite;

        // Solution solucaoBRKGA = BRKGA(instance, numGeracoesBRKGA, tamanhoPopulacaoBRKGA,
        //                                        numElite, mutantes, probElite);

        // solucaoBRKGA.imprime(instance);

        std::cout << "========VNS========" << "\n";
        Solution solucaoVNS = VNS(solucaoSWAP, instance, 12, 600.0, 200.0, 0.9995, 100);
        solucaoVNS.imprime(instance);
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}