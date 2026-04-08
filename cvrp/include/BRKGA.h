#pragma once

#include "VRPInstance.h"
#include "Solution.h"

Solution BRKGA(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int numElite, double mutantes, double probElite);