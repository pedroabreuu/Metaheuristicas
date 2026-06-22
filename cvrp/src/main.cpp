#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <functional>
#include <map>
#include "utils/parser.h"
#include "utils/CWSavings.h"
#include "utils/rng.h"
#include "utils/experimento.h"
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

using Algoritmo = std::function<Solution(const VRPInstance&, double)>;

static const std::map<std::string, Algoritmo>& registro() {
    static const std::map<std::string, Algoritmo> algos = {
        {"sa", [](const VRPInstance& inst, double tempo) {
            return SimulatedAnnealing(solucaoInicial(inst), inst, tempo > 0 ? tempo : 300.0, 2000, 0.9995);
        }},
        {"ga", [](const VRPInstance& inst, double tempo) {
            int ger = tempo > 0 ? std::numeric_limits<int>::max() : 5000;
            return GeneticAlgorithm(inst, ger, 120, 3, 2, 0.15, 0.8, tempo);
        }},
        {"brkga", [](const VRPInstance& inst, double tempo) {
            int ger = tempo > 0 ? std::numeric_limits<int>::max() : 2500;
            return BRKGA(inst, ger, 100, 10, 0.10, 0.70, tempo);
        }},
        {"vns", [](const VRPInstance& inst, double tempo) {
            return VNS(solucaoInicial(inst), inst, 18, tempo > 0 ? tempo : 200.0, 200.0, 0.999, 200);
        }},
        {"grasp", [](const VRPInstance& inst, double tempo) {
            return GRASP(inst, tempo > 0 ? tempo : 900.0);
        }},
        {"ils", [](const VRPInstance& inst, double tempo) {
            return ILS(inst, tempo > 0 ? tempo : 180.0, 500, 1, 4, 14, true);
        }},
        {"lns", [](const VRPInstance& inst, double tempo) {
            return LNS(inst, tempo > 0 ? tempo : 720.0, 1000, 0.1, 0.5, 150.0, 0.995);
        }},
        {"pso", [](const VRPInstance& inst, double tempo) {
            return PSO(inst, tempo > 0 ? tempo : 300.0, 30, 0.73, 1.5, 1.5, 0.3);
        }},
    };
    return algos;
}

static void uso(const char* prog) {
    std::cerr << "uso: " << prog << " <instancia.vrp> <algoritmo> [seed] [--tempo s] [--csv arquivo] [--traj arquivo]\n";
    std::cerr << "  seed \n";
    std::cerr << "  --tempo s \n";
    std::cerr << "  --csv arquivo \n";
    std::cerr << "  --traj arquivo grava (tempo,custo) para TTT\n";
    std::cerr << "metaheuristicas: ";
    bool primeiro = true;
    for (const auto& [nome, _] : registro()) {
        std::cerr << (primeiro ? "" : ", ") << nome;
        primeiro = false;
    }
    std::cerr << "\nexemplo: " << prog << " data/A/A-n32-k5.vrp lns 42 --tempo 300 --csv resultados.csv\n";
}

static std::string nomeInstancia(const std::string& filepath) {
    size_t barra = filepath.find_last_of('/');
    std::string nome = (barra == std::string::npos) ? filepath : filepath.substr(barra + 1);
    size_t ponto = nome.rfind(".vrp");
    if (ponto != std::string::npos) nome = nome.substr(0, ponto);
    return nome;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        uso(argv[0]);
        return 1;
    }

    std::string filepath = argv[1];
    std::string algo = argv[2];

    unsigned int seed = std::random_device{}();
    bool seedInformada = false;
    double tempo = 0.0;
    std::string arquivoCsv;
    std::string arquivoTraj;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tempo" && i + 1 < argc) {
            tempo = std::stod(argv[++i]);
        } else if (arg == "--csv" && i + 1 < argc) {
            arquivoCsv = argv[++i];
        } else if (arg == "--traj" && i + 1 < argc) {
            arquivoTraj = argv[++i];
        } else if (!seedInformada && arg.rfind("--", 0) != 0) {
            seed = static_cast<unsigned int>(std::stoul(arg));
            seedInformada = true;
        } else {
            std::cerr << "erro: argumento desconhecido '" << arg << "'\n";
            uso(argv[0]);
            return 1;
        }
    }

    auto it = registro().find(algo);
    if (it == registro().end()) {
        std::cerr << "erro: metaheuristica desconhecida '" << algo << "'\n";
        uso(argv[0]);
        return 1;
    }

    rngSetSeed(seed);

    try {
        VRPInstance instance = parseVRP(filepath);
        std::cout << "otimo conhecido: " << instance.optimal_value << "\n";
        std::cout << "seed: " << seed << "\n";
        std::cout << "======== " << algo << " ========\n";

        expInicia(arquivoTraj);
        auto inicio = std::chrono::steady_clock::now();
        Solution resultado = it->second(instance, tempo);
        double tempoTotal = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();

        resultado.calculaCusto(instance);
        resultado.imprime(instance);

        if (!arquivoCsv.empty()) {
            std::ofstream csv(arquivoCsv, std::ios::app);
            if (!csv) {
                std::cerr << "erro: nao foi possivel abrir '" << arquivoCsv << "'\n";
                return 1;
            }
            csv << nomeInstancia(filepath) << ',' << algo << ',' << seed << ','
                << resultado.custoTotal << ',';
            if (instance.optimal_value > 0) {
                double gap = 100.0 * (resultado.custoTotal - instance.optimal_value)
                           / static_cast<double>(instance.optimal_value);
                csv << gap;
            }
            csv << ',' << expTempoMelhor() << ',' << tempoTotal << '\n';
        }
    }
    catch (const std::exception& e) {
        std::cerr << "erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
