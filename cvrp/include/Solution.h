#pragma once
#include <vector> 
#include "VRPInstance.h"

struct Solution { 
  std::vector<std::vector<int>> rotas;
  int custoTotal = 0;

  void calculaCusto(const VRPInstance& instance);
  void imprime(const VRPInstance& instance) const;
};
