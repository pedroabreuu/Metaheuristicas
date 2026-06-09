#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution ACO(const VRPInstance& instance, double tempoLimiteSegundos, double alpha, double beta, double rho, double Q, double tauInicial);