#pragma once

#include <vector>
#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution decodeRandomKeys(const std::vector<double>& keys, const VRPInstance& instance);
void encodeRandomKeys(const Solution& solucao, const VRPInstance& instance, std::vector<double>& keys);
Solution decodeSemBuscaLocal(const std::vector<double>& keys, const VRPInstance& instance);
Solution decodeEBuscaLocal(const std::vector<double>& keys, const VRPInstance& instance);
