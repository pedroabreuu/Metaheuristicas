#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution BRKGA(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int numElite, double mutantes, double probElite, bool lsApenasElite);
