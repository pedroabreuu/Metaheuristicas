#include <vector>
#include <algorithm>
#include "relocate.h"

static const Node& buscaNode(const VRPInstance& instance, int id) {
    for (const auto& node : instance.nodes) {
        if (node.id == id) return node;
    }
    throw std::runtime_error("Nó não encontrado");
}

Solution relocate(Solution solucao, const VRPInstance& instance) {
  const Node& depot = instance.getDepot();
  bool melhorou = true;
  while(melhorou) {
    melhorou = false;
    for (size_t i = 0; i < solucao.rotas.size(); i++) {
      for (size_t j = 0; j < solucao.rotas[i].size(); j++) {
        double melhorMelhoria = 0;
        int melhorRotaDestino = -1;
        int melhorPosInsercao = -1;

        const Node& clienteNode = buscaNode(instance, solucao.rotas[i][j]);
        const Node& antesOrigem = (j == 0) ? depot : buscaNode(instance, solucao.rotas[i][j-1]);
        const Node& depoisOrigem = (j == solucao.rotas[i].size() - 1) ? depot : buscaNode(instance, solucao.rotas[i][j+1]);
        double economiaRemocao = instance.distancia(antesOrigem, clienteNode) 
                                + instance.distancia(clienteNode, depoisOrigem) 
                                - instance.distancia(antesOrigem, depoisOrigem);

        for (size_t k = 0; k < solucao.rotas.size(); k++) {
          if (k == i) continue;
          int demandaTotal = 0;
          int demandaCliente = buscaNode(instance, solucao.rotas[i][j]).demanda;
          for (int id: solucao.rotas[k]) {
            for (const auto& node: instance.nodes) {
              if (node.id == id) {
                demandaTotal += node.demanda;
                break;
              }
            }
          }
          if (demandaTotal + demandaCliente > instance.capacity) continue;
          for (size_t l = 0; l <= solucao.rotas[k].size(); l++) {
            const Node& antesDestino = (l == 0) ? depot : buscaNode(instance, solucao.rotas[k][l-1]);
            const Node& depoisDestino = (l == solucao.rotas[k].size()) ? depot : buscaNode(instance, solucao.rotas[k][l]);
            double custoInsercao = instance.distancia(antesDestino, clienteNode) 
                          + instance.distancia(clienteNode, depoisDestino) 
                          - instance.distancia(antesDestino, depoisDestino);
            double melhoria = economiaRemocao - custoInsercao;
            if (melhoria > melhorMelhoria) {
              melhorMelhoria = melhoria;
              melhorRotaDestino = k;
              melhorPosInsercao = l;
            }
          }
        }
        if (melhorRotaDestino != -1) {
          int clienteId = solucao.rotas[i][j];
          solucao.rotas[i].erase(solucao.rotas[i].begin() + j);
          solucao.rotas[melhorRotaDestino].insert(
              solucao.rotas[melhorRotaDestino].begin() + melhorPosInsercao, clienteId);
          melhorou = true;
          break;  
        }
      }
      if (melhorou) break;  
    }
  }
  return solucao;
}

Solution randomRelocate(Solution solucao, const VRPInstance& instance) {
  return solucao;
}