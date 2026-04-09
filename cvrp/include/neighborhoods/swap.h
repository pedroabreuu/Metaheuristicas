#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution swapIntra(Solution solucao, const VRPInstance& instance);
Solution swapInter(Solution solucao, const VRPInstance& instance);