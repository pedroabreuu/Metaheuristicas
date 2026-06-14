#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution PSO(const VRPInstance& instance, double tempoLimite, int numParticulas, double w, double c1, double c2, double vMax);
