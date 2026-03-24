#pragma once

#include "VRPInstance.h"
#include "Solution.h"

Solution SimulatedAnnealing(Solution solucao, const VRPInstance& instance, double To, int SAmax, double alpha);
