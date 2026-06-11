#include "utils/rng.h"

std::mt19937& rngGlobal() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

void rngSetSeed(unsigned int seed) {
    rngGlobal().seed(seed);
}
