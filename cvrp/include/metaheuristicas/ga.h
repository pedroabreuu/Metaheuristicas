#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution GeneticAlgorithm(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int tamanhoTorneio, int elitismo, double probMutacao, double probCrossover, double tempoLimite = 0.0);
