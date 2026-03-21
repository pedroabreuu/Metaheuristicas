#include <vector>
#include <algorithm> 
#include <limits>
#include "nearestNeighbor.h"

Solution nearestN(const VRPInstance& instance) {
  std::vector<bool> visitados(instance.nodes.size(), false);

  for (size_t i = 0; i < instance.nodes.size(); i++) {
    if (instance.nodes[i].id == instance.depot_id) {
        visitados[i] = true;
        break;
    }
  }

  Solution solucao;

  int totalClientes = instance.nodes.size() - 1;
  int clientesVisitados = 0;

  while(clientesVisitados < totalClientes) {
    std::vector<int> rota;
    int capacidadeRestante = instance.capacity;
    const Node* anterior = &instance.getDepot();

    while(true) {
      double menorDistancia = std::numeric_limits<double>::max();
      int melhorCandidato = -1;  // indice do melhor node candidatoa
      for (size_t i = 0; i < instance.nodes.size(); i++) {
        if (visitados[i] == false && capacidadeRestante >= instance.nodes[i].demanda && instance.distancia(instance.nodes[i], *anterior) < menorDistancia) {
              melhorCandidato = i;
              menorDistancia = instance.distancia(instance.nodes[i], *anterior);
        }
      }
      if (melhorCandidato != -1) {
        rota.push_back(instance.nodes[melhorCandidato].id); 
        visitados[melhorCandidato] = true;
        capacidadeRestante = capacidadeRestante - instance.nodes[melhorCandidato].demanda;
        anterior = &instance.nodes[melhorCandidato];
        clientesVisitados += 1;
      }
      else { break; }
    }
    solucao.rotas.push_back(rota);
  }
  return solucao;
}
