#include <vector>
#include <algorithm>
#include <random>
#include "neighborhoods/relocate.h"

Solution relocate(Solution solucao, const VRPInstance& instance) {
  const Node& depot = instance.getDepot();
  const auto& nodes = instance.nodes;
  const auto& mapa = instance.mapa;

  auto nodeById = [&](int id) -> const Node& {
    return nodes[mapa.at(id)];
  };

  std::vector<int> cargaRotas(solucao.rotas.size(), 0);
  for (size_t r = 0; r < solucao.rotas.size(); ++r) {
    for (int id : solucao.rotas[r]) {
      cargaRotas[r] += nodeById(id).demanda;
    }
  }

  bool melhorou = true;
  while (melhorou) {
    melhorou = false;

    for (size_t i = 0; i < solucao.rotas.size() && !melhorou; i++) {
      if (solucao.rotas[i].empty()) continue; // ADICIONADO

      for (size_t j = 0; j < solucao.rotas[i].size() && !melhorou; j++) {
        double melhorMelhoria = 0.0;
        int melhorRotaDestino = -1;
        int melhorPosInsercao = -1;

        const int clienteId = solucao.rotas[i][j];
        const Node& clienteNode = nodeById(clienteId);
        const int demandaCliente = clienteNode.demanda;

        const Node& antesOrigem =
            (j == 0) ? depot : nodeById(solucao.rotas[i][j - 1]);
        const Node& depoisOrigem =
            (j + 1 == solucao.rotas[i].size()) ? depot
                                               : nodeById(solucao.rotas[i][j + 1]);

        const double economiaRemocao =
            instance.distancia(antesOrigem, clienteNode)
          + instance.distancia(clienteNode, depoisOrigem)
          - instance.distancia(antesOrigem, depoisOrigem);

        for (size_t k = 0; k < solucao.rotas.size(); k++) {
          if (k == i) continue;

          if (cargaRotas[k] + demandaCliente > instance.capacity) continue;

          for (size_t l = 0; l <= solucao.rotas[k].size(); l++) {
            const Node& antesDestino =
                (l == 0) ? depot : nodeById(solucao.rotas[k][l - 1]);
            const Node& depoisDestino =
                (l == solucao.rotas[k].size()) ? depot
                                               : nodeById(solucao.rotas[k][l]);

            const double custoInsercao =
                instance.distancia(antesDestino, clienteNode)
              + instance.distancia(clienteNode, depoisDestino)
              - instance.distancia(antesDestino, depoisDestino);

            const double melhoria = economiaRemocao - custoInsercao;

            if (melhoria > melhorMelhoria) {
              melhorMelhoria = melhoria;
              melhorRotaDestino = static_cast<int>(k);
              melhorPosInsercao = static_cast<int>(l);
            }
          }
        }

        if (melhorRotaDestino != -1) {
          solucao.rotas[i].erase(solucao.rotas[i].begin() + j);
          solucao.rotas[melhorRotaDestino].insert(
              solucao.rotas[melhorRotaDestino].begin() + melhorPosInsercao, clienteId);

          cargaRotas[i] -= demandaCliente;
          cargaRotas[melhorRotaDestino] += demandaCliente;

          if (solucao.rotas[i].empty()) {
            solucao.rotas.erase(solucao.rotas.begin() + i);
            cargaRotas.erase(cargaRotas.begin() + i);
          }

          melhorou = true;
        }
      }
    }
  }

  return solucao;
}

Solution randomRelocate(Solution solucao, const VRPInstance& instance) {
  static std::mt19937 gen(std::random_device{}());

  if (solucao.rotas.size() < 2) {
    return solucao;
  }

  std::uniform_int_distribution<> rotaOrigem(0, static_cast<int>(solucao.rotas.size()) - 1);
  int origem = rotaOrigem(gen);

  if (solucao.rotas[origem].empty()) {
    return solucao;
  }

  std::uniform_int_distribution<> rotaDestino(0, static_cast<int>(solucao.rotas.size()) - 2);
  int destino = rotaDestino(gen);

  if (destino >= origem) {
    destino += 1;
  }

  std::uniform_int_distribution<> rotaCliente(0, static_cast<int>(solucao.rotas[origem].size()) - 1);
  std::uniform_int_distribution<> posInsercao(0, static_cast<int>(solucao.rotas[destino].size()));

  int cliente = rotaCliente(gen);
  int insercao = posInsercao(gen);
  int clienteId = solucao.rotas[origem][cliente];

  solucao.rotas[origem].erase(solucao.rotas[origem].begin() + cliente);
  solucao.rotas[destino].insert(solucao.rotas[destino].begin() + insercao, clienteId);

  if (solucao.rotas[origem].empty()) {
    solucao.rotas.erase(solucao.rotas.begin() + origem);
  }

  return solucao;
}