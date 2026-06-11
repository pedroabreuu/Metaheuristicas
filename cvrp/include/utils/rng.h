#pragma once

#include <random>

std::mt19937& rngGlobal();
void rngSetSeed(unsigned int seed);
