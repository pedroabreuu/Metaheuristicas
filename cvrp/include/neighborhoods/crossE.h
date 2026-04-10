#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution crossExchange(Solution solucao, const VRPInstance& instance);
Solution randomCrossExchange(Solution solucao, const VRPInstance& instance);