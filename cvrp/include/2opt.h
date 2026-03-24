#pragma once

#include "VRPInstance.h"
#include "Solution.h"

Solution opt2(Solution solucao, const VRPInstance& instance);
Solution randomOpt2(Solution solucao, const VRPInstance& instance);