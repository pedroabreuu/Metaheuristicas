#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution ILS(
    const VRPInstance& instance,
    double tempoLimite,
    int iterSemMelhoraMax,
    int Bmin = 1,
    int Bmax = 3,
    int BmaxStuck = 5,
    bool usarRVND = true
);
