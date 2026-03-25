#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include "2opt.h"

Solution opt2(Solution solucao, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();

    for (size_t i = 0; i < solucao.rotas.size(); i++) {
      bool melhorou = true;
      while(melhorou) {
        melhorou = false;
        for (size_t j = 0; j < solucao.rotas[i].size() - 1; j++) {
          for (size_t k = j+1; k < solucao.rotas[i].size(); k++) {
            const Node& noA = (j == 0) ? depot : instance.getNode(solucao.rotas[i][j-1]);
            const Node& noB = instance.getNode(solucao.rotas[i][j]);
            const Node& noC = instance.getNode(solucao.rotas[i][k]);
            const Node& noD = (k == solucao.rotas[i].size() - 1) ? depot : instance.getNode(solucao.rotas[i][k+1]);
            
            double antes  = instance.distancia(noA, noB) + instance.distancia(noC, noD);
            double depois = instance.distancia(noA, noC) + instance.distancia(noB, noD);

            if (depois < antes) {
              std::reverse(solucao.rotas[i].begin() + j, solucao.rotas[i].begin() + k + 1);
              melhorou = true;
            }
          }
        }
      }
    }

    return solucao;
}

Solution randomOpt2(Solution solucao, const VRPInstance& instance) {
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> rota(0, solucao.rotas.size()-1);

  int r = rota(gen);

  if (solucao.rotas[r].size() >= 2) {
    std::uniform_int_distribution<> verificacao(0, solucao.rotas[r].size() - 1);
    int j = verificacao(gen);
    int k = verificacao(gen);

    if (j == k) {
      while(j == k) { k = verificacao(gen); }
    }

    if (j > k) {
      std::swap(j, k);
    }

    std::reverse(solucao.rotas[r].begin() + j, solucao.rotas[r].begin() + k + 1);

  } else { return solucao; }


  return solucao;
}