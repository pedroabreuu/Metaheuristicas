#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution relocate(Solution solucao, const VRPInstance& instance);
Solution randomRelocate(Solution solucao, const VRPInstance& instance);
