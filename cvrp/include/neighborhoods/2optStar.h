#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution opt2Star(Solution solucao, const VRPInstance& instance);
Solution randomOpt2Star(Solution solucao, const VRPInstance& instance);
