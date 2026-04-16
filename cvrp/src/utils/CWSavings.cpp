#include <vector>
#include <algorithm>
#include <limits>
#include "utils/CWSavings.h"

struct Saving {
  int i;
  int j;
  double valor;
};

Solution clarkeWright(const VRPInstance& instance) {
  Solution solucao;

  // inicializando todos os nos com rotas individuais
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

      for (int id : solucao.rotas[rotaI]) {
        demandaTotal += instance.getNode(id).demanda;
      }

      for (int id : solucao.rotas[rotaJ]) {
        demandaTotal += instance.getNode(id).demanda;
      }

      if (demandaTotal <= instance.capacity) {
        bool iNoBack  = (savings[i].i == solucao.rotas[rotaI].back());
        bool jNoFront = (savings[i].j == solucao.rotas[rotaJ].front());

        if (iNoBack && jNoFront) {
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else if (!iNoBack && !jNoFront) {
            std::reverse(solucao.rotas[rotaI].begin(), solucao.rotas[rotaI].end());
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else if (iNoBack && !jNoFront) {
            std::reverse(solucao.rotas[rotaJ].begin(), solucao.rotas[rotaJ].end());
            solucao.rotas[rotaI].insert(
                solucao.rotas[rotaI].end(),
                solucao.rotas[rotaJ].begin(),
                solucao.rotas[rotaJ].end()
            );
        }
        else {
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

  while (instance.num_trucks > 0 &&
         static_cast<int>(solucao.rotas.size()) > instance.num_trucks) {

    double melhorCusto = std::numeric_limits<double>::max();
    int melhorI = -1, melhorJ = -1;
    bool melhorReverseI = false, melhorReverseJ = false;

    auto custoMerge = [&](const std::vector<int>& rA, const std::vector<int>& rB) -> double {
      // custo de ligar o final de rA ao inicio de rB
      const Node& ultimo = instance.getNode(rA.back());
      const Node& primeiro = instance.getNode(rB.front());
      const Node& dep = instance.getDepot();
      // economia: removemos dep<-ultimo e primeiro->dep, adicionamos ultimo->primeiro
      return instance.distancia(ultimo, primeiro)
           - instance.distancia(ultimo, dep)
           - instance.distancia(dep, primeiro);
    };

    for (size_t i = 0; i < solucao.rotas.size(); i++) {
      for (size_t j = i + 1; j < solucao.rotas.size(); j++) {
        // verificar capacidade
        int demI = 0, demJ = 0;
        for (int id : solucao.rotas[i]) demI += instance.getNode(id).demanda;
        for (int id : solucao.rotas[j]) demJ += instance.getNode(id).demanda;

        if (demI + demJ > instance.capacity) continue;

        // testar 4 orientações de merge e pegar a melhor
        std::vector<int> rI = solucao.rotas[i];
        std::vector<int> rJ = solucao.rotas[j];
        std::vector<int> rI_rev(rI.rbegin(), rI.rend());
        std::vector<int> rJ_rev(rJ.rbegin(), rJ.rend());

        struct Opcao { double custo; bool revI; bool revJ; };
        Opcao opcoes[4] = {
          { custoMerge(rI,     rJ),     false, false },
          { custoMerge(rI,     rJ_rev), false, true  },
          { custoMerge(rI_rev, rJ),     true,  false },
          { custoMerge(rI_rev, rJ_rev), true,  true  },
        };

        for (auto& op : opcoes) {
          if (op.custo < melhorCusto) {
            melhorCusto = op.custo;
            melhorI = static_cast<int>(i);
            melhorJ = static_cast<int>(j);
            melhorReverseI = op.revI;
            melhorReverseJ = op.revJ;
          }
        }
      }
    }


    if (melhorI == -1) {
      // encontrar as duas rotas com menor demanda combinada
      int minDem = std::numeric_limits<int>::max();
      for (size_t i = 0; i < solucao.rotas.size(); i++) {
        for (size_t j = i + 1; j < solucao.rotas.size(); j++) {
          int dem = 0;
          for (int id : solucao.rotas[i]) dem += instance.getNode(id).demanda;
          for (int id : solucao.rotas[j]) dem += instance.getNode(id).demanda;
          if (dem < minDem) {
            minDem = dem;
            melhorI = static_cast<int>(i);
            melhorJ = static_cast<int>(j);
          }
        }
      }
      melhorReverseI = false;
      melhorReverseJ = false;
    }

    if (melhorReverseI) {
      std::reverse(solucao.rotas[melhorI].begin(), solucao.rotas[melhorI].end());
    }
    if (melhorReverseJ) {
      std::reverse(solucao.rotas[melhorJ].begin(), solucao.rotas[melhorJ].end());
    }

    solucao.rotas[melhorI].insert(
      solucao.rotas[melhorI].end(),
      solucao.rotas[melhorJ].begin(),
      solucao.rotas[melhorJ].end()
    );
    solucao.rotas.erase(solucao.rotas.begin() + melhorJ);
  }

  return solucao;
}