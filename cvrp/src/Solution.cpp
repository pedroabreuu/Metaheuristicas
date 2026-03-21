#include <Solution.h> 
#include <iostream>

void Solution::calculaCusto(const VRPInstance& instance) {
  custoTotal = 0;
  const Node& depot = instance.getDepot();

  for (const auto& rota: rotas) {
    // comeca no depot
    const Node* anterior = &depot;
    for (int id: rota) { // para cada node encontrado, acumula a distancia
      for (const auto& node: instance.nodes) {
        if (node.id == id) {
          custoTotal += instance.distancia(*anterior, node);
          anterior = &node;
          break;
        }
      }
    }
    // voltando pro deposito
    custoTotal += instance.distancia(*anterior, depot);
  }
}

void Solution::imprime(const VRPInstance& instance) const {
  int depot = instance.depot_id;
  
  for (size_t i = 0; i < rotas.size(); i++) {
    std::cout << "Route" << "#" << i + 1<< ": ";
    std::cout << depot - 1;

    for (size_t j = 0; j < rotas[i].size(); j++) {
      std::cout << " " <<rotas[i][j] - 1;  
    }

    std::cout << " " << depot - 1 << std::endl;
  }

  std::cout << "Cost " << custoTotal << std::endl;
}


