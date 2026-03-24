#pragma once

#include "VRPInstance.h"
#include "Solution.h"

Solution relocate(Solution solucao, const VRPInstance& instance);
Solution randomRelocate(Solution solucao, const VRPInstance& instance);
