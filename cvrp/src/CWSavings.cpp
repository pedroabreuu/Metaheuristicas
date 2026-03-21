#include <vector>
#include <algorithm>
#include "CWSavings.h"

struct Saving {
  int i;
  int j;
  double valor;
};

Solution clarkeWright(const VRPInstance& instance) {
  Solution solucao;

  // inicializando dos os nos com rotas
  for (size_t i = 0; i < instance.nodes.size(); i++) {
    if (instance.nodes[i].id != instance.depot_id) {
      solucao.rotas.push_back({instance.nodes[i].id});
    }
  }

  // calcular e ordenar savings  
  std::vector<Saving> savings;
  const Node* deposito = &instance.getDepot();
  Saving s;
  for (size_t i = 0; i < instance.nodes.size(); i++) {
    s.i = instance.nodes[i].id;
    for (size_t j = i+1; j < instance.nodes.size(); j++) {
      s.j = instance.nodes[j].id;
      if (s.i != instance.depot_id && s.j != instance.depot_id) {
        s.valor = instance.distancia(instance.nodes[i], *deposito) + instance.distancia(*deposito, instance.nodes[j]) - instance.distancia(instance.nodes[i], instance.nodes[j]);
        savings.push_back(s);
      }
    }
  }

  std::sort(savings.begin(), savings.end(), [](const Saving& a, const Saving& b) {
    return a.valor > b.valor;
  });

  // percorrer savings e unir rotas
  for (size_t i = 0; i < savings.size(); i++){
    int rotaI = -1;
    int rotaJ = -1;
    for (size_t j = 0; j < solucao.rotas.size(); j++) {
      if (savings[i].i == solucao.rotas[j].front() || savings[i].i == solucao.rotas[j].back()) { rotaI = j; }
      if (savings[i].j == solucao.rotas[j].front() || savings[i].j == solucao.rotas[j].back()) { rotaJ = j; }
    } 
    if (rotaI != -1 && rotaJ != -1 && rotaI != rotaJ) {
      int demandaTotal = 0;

      for (int id : solucao.rotas[rotaI]) {       // para cada cliente na rota I
      for (const auto& node : instance.nodes) { // procura o nó com esse ID
          if (node.id == id) {
              demandaTotal += node.demanda;
              break;
          }
        }
      }

      for (int id : solucao.rotas[rotaJ]) {       // para cada cliente na rota I
        for (const auto& node : instance.nodes) { // procura o nó com esse ID
            if (node.id == id) {
                demandaTotal += node.demanda;
                break;
            }
        }
      }
      if (demandaTotal <= instance.capacity) {
        bool iNoBack  = (savings[i].i == solucao.rotas[rotaI].back());
        bool jNoFront = (savings[i].j == solucao.rotas[rotaJ].front());

        if (iNoBack && jNoFront) {
            // i no final da rota I, j no início da rota J
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else if (!iNoBack && !jNoFront) {
            // i no início da rota I, j no final da rota J
            std::reverse(solucao.rotas[rotaI].begin(), solucao.rotas[rotaI].end());
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else if (iNoBack && !jNoFront) {
            // ambos no final das suas rotas
            std::reverse(solucao.rotas[rotaJ].begin(), solucao.rotas[rotaJ].end());
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else {
            // i no início da rota I, j no início da rota J
            std::reverse(solucao.rotas[rotaI].begin(), solucao.rotas[rotaI].end());
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }

        solucao.rotas.erase(solucao.rotas.begin() + rotaJ);
      }
    }
  }

  return solucao;
}


