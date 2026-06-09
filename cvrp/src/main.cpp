#include <iostream>
#include <chrono>
#include <iomanip>
#include "utils/parser.h"
#include "utils/nearestNeighbor.h"
#include "utils/CWSavings.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"
#include "metaheuristicas/SA.h"
#include "metaheuristicas/ga.h"
#include "metaheuristicas/BRKGA.h"
#include "metaheuristicas/vns.h"
#include "metaheuristicas/grasp.h"
#include "metaheuristicas/ils.h"
#include "metaheuristicas/lns.h"

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

        // auto t0 = std::chrono::steady_clock::now();

        // // Solution solucaoNN = nearestN(instance);
        // // solucaoNN.calculaCusto(instance);
        // // std::cout << "========Nearest Neighbor========" << "\n";
        // // solucaoNN.imprime(instance);

        // Solution solucaoCW = clarkeWright(instance);
        // solucaoCW.calculaCusto(instance);

        // std::vector<Solution (*)(Solution, const VRPInstance&)> buscasLocais = {
        //     crossExchange,
        //     opt2,
        //     relocate,
        //     orOptIntra3,
        //     opt2Star,
        //     orOptInter3,
        //     swapIntra,
        //     swapInter,
        //     crossExchange,
        //     opt2Star
        // };

        // Solution solucaoFinal = solucaoCW;
        // int k = 0;
        // while (k < static_cast<int>(buscasLocais.size())) {
        //     Solution candidata = buscasLocais[k](solucaoFinal, instance);
        //     candidata.calculaCusto(instance);

        //     if (candidata.custoTotal < solucaoFinal.custoTotal) {
        //         solucaoFinal = candidata;
        //         k = 0;
        //     } else {
        //         k++;
        //     }
        // }

        // auto t1 = std::chrono::steady_clock::now();
        // double tempoCW = std::chrono::duration<double>(t1 - t0).count();

        // std::cout << "========Clarke-Wright + VND========" << "\n";
        // std::cout << "Custo inicial CW: " << solucaoCW.custoTotal << "\n";
        // std::cout << "Custo final: " << solucaoFinal.custoTotal
        //           << " em " << tempoCW << "s" << std::endl;
        // solucaoFinal.imprime(instance);

        // Solution solucaoSA = SimulatedAnnealing(solucaoFinal, instance, 300.0, 2000, 0.9995);
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
         
        // Solution solucaoGA = GeneticAlgorithm(instance, 5000, 120,
        //                                        3, 2, 0.15, 0.8);
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

        // Solution solucaoBRKGA = BRKGA(instance, 2500, 100, 10, 0.10, 0.70);

        // solucaoBRKGA.imprime(instance);

        // std::cout << "========VNS========" << "\n";
        // Solution solucaoVNS = VNS(solucaoSWAP, instance, 18, 200.0, 200.0, 0.999, 200);
        // solucaoVNS.imprime(instance);

        // std::cout << "========GRASP========" << "\n";
        // Solution solucaoGRASP = GRASP(instance, 900.0);
        // solucaoGRASP.imprime(instance);

        // std::cout << "========ILS========" << "\n";
        // Solution solucaoVNS = ILS(instance, 180.0, 150, 1, 3, 8);
        // solucaoVNS.imprime(instance);

        std::cout << "========LNS========" << "\n";
        Solution solucaoLNS = LNS(instance, 720.0, 1000, 0.1, 0.5, 150.0, 0.995);
        solucaoLNS.imprime(instance);
        
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}