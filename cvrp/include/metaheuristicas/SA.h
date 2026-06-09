#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double tempoLimiteSegundos, int SAmax, double alpha);
