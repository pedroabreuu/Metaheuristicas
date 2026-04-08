#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include "BRKGA.h"
#include "relocate.h"
#include "2opt.h"

struct Cromossomo {
    std::vector<double> keys;
    Solution solution;
    double fitness = std::numeric_limits<double>::infinity();
};

static Cromossomo gerarCromossomoAleatorio(const VRPInstance& instance, std::mt19937& gen) {
    std::uniform_real_distribution<> chaves(0.0,1.0);
    Cromossomo cromossomo;

    for (size_t i = 1; i < instance.nodes.size(); i++) {
        cromossomo.keys.push_back(chaves(gen));
    }

    return cromossomo;
}

static void decoder(Cromossomo& cromossomo, const VRPInstance& instance) {
    

}

static Cromossomo crossoverBiased(const Cromossomo& elite, const Cromossomo& naoElite, double probElite, std::mt19937& gen) {
    return cromossomo;
}

Solution BRKGA(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int numElite, double mutantes, double probElite) {

}