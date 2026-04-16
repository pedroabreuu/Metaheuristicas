#include "core/Solution.h" 
#include <iostream>

void Solution::calculaCusto(const VRPInstance& instance) {
    custoTotal = 0;
    const Node& depot = instance.getDepot();

    for (const auto& rota : rotas) {
        int demandaTotal = 0;
        const Node* anterior = &depot;
        for (int id : rota) {
            const Node& node = instance.getNode(id);
            demandaTotal += node.demanda;
            custoTotal += instance.distancia(*anterior, node);
            anterior = &node;
        }
        custoTotal += instance.distancia(*anterior, depot);

        if (demandaTotal > instance.capacity) {
            custoTotal += 1000000 * (demandaTotal - instance.capacity);
        }
    }

    if (instance.num_trucks > 0 && 
        static_cast<int>(rotas.size()) > instance.num_trucks) {
        custoTotal += 1000000 * (rotas.size() - instance.num_trucks);
    }
}

int Solution::getCusto() const {
    return custoTotal;
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


