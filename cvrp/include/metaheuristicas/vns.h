#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution VND(Solution solucao, const VRPInstance& instance);
Solution perturbar(Solution solucao, const VRPInstance& instance, int k);
Solution VNS(Solution solucao, const VRPInstance& instance, int kMax, double tempoLimite, int iterSemMelhoraMax);
Solution VNS(Solution solucao, const VRPInstance& instance, double tempoLimite, int iterSemMelhoraMax);