#include <iostream>
#include "parser.h"
#include "nearestNeighbor.h"
#include "CWSavings.h"
#include "2opt.h"
#include "relocate.h"

int main(int argc, char* argv[]) {
    std::string filepath = (argc > 1) ? argv[1] : "data/A-n32-k5.vrp";

    try {
        VRPInstance instance = parseVRP(filepath);
        Solution solucaoNN = nearestN(instance);
        solucaoNN.calculaCusto(instance);
        Solution solucaoCW = clarkeWright(instance);
        solucaoCW.calculaCusto(instance);
        Solution solucao2opt = opt2(solucaoCW, instance);
        solucao2opt.calculaCusto(instance);
        Solution solucaoRelocate = relocate(solucao2opt, instance);
        solucaoRelocate.calculaCusto(instance);

        std::cout << "Instância carregada.\n";
        std::cout << "Depot ID:     " << instance.depot_id   << "\n";
        std::cout << "Capacidade:   " << instance.capacity   << "\n";
        std::cout << "Nós:          " << instance.nodes.size() << "\n";
        std::cout << "Caminhões:    " << instance.num_trucks  << "\n";

        std::cout << "========Nearest Neighbor========" << "\n";
        solucaoNN.imprime(instance);
        
        std::cout << "========Clarke-Wright========" << "\n";
        solucaoCW.imprime(instance);

        std::cout << "========2-opt========" << "\n";
        solucao2opt.imprime(instance);

        std::cout << "========Relocate========" << "\n";
        solucaoRelocate.imprime(instance);

    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
