#include <iostream>
#include <string>
#include <functional>
#include <map>
#include "utils/parser.h"
#include "utils/CWSavings.h"
#include "metaheuristicas/SA.h"
#include "metaheuristicas/ga.h"
#include "metaheuristicas/BRKGA.h"
#include "metaheuristicas/vns.h"
#include "metaheuristicas/grasp.h"
#include "metaheuristicas/ils.h"
#include "metaheuristicas/lns.h"
#include "metaheuristicas/pso.h"

static Solution solucaoInicial(const VRPInstance& instance) {
    Solution s = clarkeWright(instance);
    s = VND(s, instance);
    s.calculaCusto(instance);
    return s;
}

using Algoritmo = std::function<Solution(const VRPInstance&)>;

static const std::map<std::string, Algoritmo>& registro() {
    static const std::map<std::string, Algoritmo> algos = {
        {"sa",    [](const VRPInstance& inst) {
            return SimulatedAnnealing(solucaoInicial(inst), inst, 300.0, 2000, 0.9995);
        }},
        {"ga", [](const VRPInstance& inst) {
            return GeneticAlgorithm(inst, 5000, 120, 3, 2, 0.15, 0.8);
        }},
        {"brkga", [](const VRPInstance& inst) {
            return BRKGA(inst, 2500, 100, 10, 0.10, 0.70, false);
        }},
        {"brkga-elite", [](const VRPInstance& inst) {
            return BRKGA(inst, 2500, 100, 10, 0.10, 0.70, true);
        }},
        {"vns", [](const VRPInstance& inst) {
            return VNS(solucaoInicial(inst), inst, 18, 200.0, 200.0, 0.999, 200);
        }},
        {"grasp", [](const VRPInstance& inst) {
            return GRASP(inst, 900.0);
        }},
        {"ils", [](const VRPInstance& inst) {
            return ILS(inst, 180.0, 150, 1, 3, 8);
        }},
        {"lns", [](const VRPInstance& inst) {
            return LNS(inst, 720.0, 1000, 0.1, 0.5, 150.0, 0.995);
        }},
        {"pso", [](const VRPInstance& inst) {
            return PSO(inst, 300.0, 30, 0.73, 1.5, 1.5, 0.3, false); 
        }},
        {"pso-gbest", [](const VRPInstance& inst) {
            return PSO(inst, 300.0, 30, 0.73, 1.5, 1.5, 0.3, true);
        }},
    };
    return algos;
}

static void uso(const char* prog) {
    std::cerr << "uso: " << prog << " <instancia.vrp> <algoritmo>\n";
    std::cerr << "metaheuristicas: ";
    bool primeiro = true;
    for (const auto& [nome, _] : registro()) {
        std::cerr << (primeiro ? "" : ", ") << nome;
        primeiro = false;
    }
    std::cerr << "\nexemplo: " << prog << " data/A/A-n32-k5.vrp lns\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        uso(argv[0]);
        return 1;
    }

    std::string filepath = argv[1];
    std::string algo = argv[2];

    auto it = registro().find(algo);
    if (it == registro().end()) {
        std::cerr << "erro: metaheuristica desconhecida '" << algo << "'\n";
        uso(argv[0]);
        return 1;
    }

    try {
        VRPInstance instance = parseVRP(filepath);
        std::cout << "otimo conhecido: " << instance.optimal_value << "\n";
        std::cout << "======== " << algo << " ========\n";
        Solution resultado = it->second(instance);
        resultado.calculaCusto(instance);
        resultado.imprime(instance);
    }
    catch (const std::exception& e) {
        std::cerr << "erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
