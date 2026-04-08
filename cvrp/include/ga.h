#pragma once

#include "VRPInstance.h"
#include "Solution.h"

Solution GeneticAlgorithm(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int tamanhoTorneio, int elitismo, double probMutacao, double probCrossover);
