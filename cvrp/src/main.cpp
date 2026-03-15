#include <iostream>
#include "../include/parser.h"

int main(int argc, char* argv[]) {
    std::string filepath = (argc > 1) ? argv[1] : "data/A-n32-k5.vrp";

    try {
        VRPInstance instance = parseVRP(filepath);

        std::cout << "Instância carregada.\n";
        std::cout << "Depot ID:     " << instance.depot_id   << "\n";
        std::cout << "Capacidade:   " << instance.capacity   << "\n";
        std::cout << "Nós:          " << instance.nodes.size() << "\n";
        std::cout << "Caminhões:    " << instance.num_trucks  << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
