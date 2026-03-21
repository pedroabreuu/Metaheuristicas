#include <vector>
#include <algorithm>
#include "2opt.h"

const Node& buscaNode(const VRPInstance& instance, int id) {
    for (const auto& node : instance.nodes) {
        if (node.id == id) return node;
    }
    throw std::runtime_error("Nó não encontrado");
}

Solution opt2(Solution solucao, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();

    for (size_t i = 0; i < solucao.rotas.size(); i++) {
      bool melhorou = true;
      while(melhorou) {
        melhorou = false;
        for (size_t j = 0; j < solucao.rotas[i].size() - 1; j++) {
          for (size_t k = j+1; k < solucao.rotas[i].size(); k++) {
            const Node& noA = (j == 0) ? depot : buscaNode(instance, solucao.rotas[i][j-1]);
            const Node& noB = buscaNode(instance, solucao.rotas[i][j]);
            const Node& noC = buscaNode(instance, solucao.rotas[i][k]);
            const Node& noD = (k == solucao.rotas[i].size() - 1) ? depot : buscaNode(instance, solucao.rotas[i][k+1]);
            
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
