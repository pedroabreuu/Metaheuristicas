#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution opt2(Solution solucao, const VRPInstance& instance);
Solution randomOpt2(Solution solucao, const VRPInstance& instance);