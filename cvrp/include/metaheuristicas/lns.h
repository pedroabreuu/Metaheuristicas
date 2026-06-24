#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution LNS(
    const VRPInstance& instance,
    double tempoLimite,
    int iterSemMelhoraMax,
    double rhoMin,
    double rhoMax,
    double tempInicial,
    double alpha
);
