#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution ILS(
    const VRPInstance& instance,
    double tempoLimite,
    double tempInicial,
    double alpha,
    int iterSemMelhoraMax,
    int Bmin = 1,
    int Bmax = 3,
    int BmaxStuck = 5,
    unsigned seed = 5489u
);
