#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution GRASP(const VRPInstance& instance, double tempoLimiteSegundos);