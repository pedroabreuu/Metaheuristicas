#include <vector>
#include <algorithm>
#include <limits>
#include <utility>
#include "utils/CWSavings.h"

struct Saving {
  int i;
  int j;
  double valor;
};

static bool empacotaPorDemanda(
    const VRPInstance& instance,
    const std::vector<int>& clientes,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas,
    size_t idx) {
  if (idx == clientes.size()) return true;

  const int cliente = clientes[idx];
  const int demanda = instance.getNode(cliente).demanda;
  int ultimaDemandaTestada = -1;

  for (size_t r = 0; r < rotas.size(); r++) {
    if (demandas[r] == ultimaDemandaTestada) continue;
    if (demandas[r] + demanda > instance.capacity) continue;

    rotas[r].push_back(cliente);
    demandas[r] += demanda;

    if (empacotaPorDemanda(instance, clientes, rotas, demandas, idx + 1)) {
      return true;
    }

    demandas[r] -= demanda;
    rotas[r].pop_back();
    ultimaDemandaTestada = demandas[r];

    if (demandas[r] == 0) break;
  }

  return false;
}

static void ordenaRotaPorVizinhoMaisProximo(
    const VRPInstance& instance,
    std::vector<int>& rota) {
  std::vector<int> naoVisitados = rota;
  rota.clear();

  const Node* atual = &instance.getDepot();
  while (!naoVisitados.empty()) {
    auto melhor = naoVisitados.begin();
    double melhorDist = std::numeric_limits<double>::max();

    for (auto it = naoVisitados.begin(); it != naoVisitados.end(); ++it) {
      double dist = instance.distancia(*atual, instance.getNode(*it));
      if (dist < melhorDist) {
        melhorDist = dist;
        melhor = it;
      }
    }

    rota.push_back(*melhor);
    atual = &instance.getNode(*melhor);
    naoVisitados.erase(melhor);
  }
}

Solution clarkeWright(const VRPInstance& instance) {
  Solution solucao;

  // rota individual para cada cliente
  std::vector<int> demandaRota;
  for (size_t i = 0; i < instance.nodes.size(); i++) {
    if (instance.nodes[i].id != instance.depot_id) {
      solucao.rotas.push_back({instance.nodes[i].id});
      demandaRota.push_back(instance.nodes[i].demanda);
    }
  }

  // lista de savings
  std::vector<Saving> savings;
  const Node& deposito = instance.getDepot();
  for (size_t i = 0; i < instance.nodes.size(); i++) {
    if (instance.nodes[i].id == instance.depot_id) continue;
    for (size_t j = i + 1; j < instance.nodes.size(); j++) {
      if (instance.nodes[j].id == instance.depot_id) continue;
      Saving s;
      s.i = instance.nodes[i].id;
      s.j = instance.nodes[j].id;
      s.valor = instance.distancia(instance.nodes[i], deposito)
              + instance.distancia(deposito, instance.nodes[j])
              - instance.distancia(instance.nodes[i], instance.nodes[j]);
      savings.push_back(s);
    }
  }

  std::sort(savings.begin(), savings.end(),
            [](const Saving& a, const Saving& b) { return a.valor > b.valor; });

  for (size_t s = 0; s < savings.size(); s++) {
    int rotaI = -1, rotaJ = -1;
    for (size_t k = 0; k < solucao.rotas.size(); k++) {
      if (savings[s].i == solucao.rotas[k].front() || savings[s].i == solucao.rotas[k].back()) rotaI = (int)k;
      if (savings[s].j == solucao.rotas[k].front() || savings[s].j == solucao.rotas[k].back()) rotaJ = (int)k;
    }
    if (rotaI == -1 || rotaJ == -1 || rotaI == rotaJ) continue;
    if (demandaRota[rotaI] + demandaRota[rotaJ] > instance.capacity) continue;

    bool iNoBack  = (savings[s].i == solucao.rotas[rotaI].back());
    bool jNoFront = (savings[s].j == solucao.rotas[rotaJ].front());

    if (iNoBack && jNoFront) {
    } else if (!iNoBack && jNoFront) {
      std::reverse(solucao.rotas[rotaI].begin(), solucao.rotas[rotaI].end());
    } else if (iNoBack && !jNoFront) {
      std::reverse(solucao.rotas[rotaJ].begin(), solucao.rotas[rotaJ].end());
    } else {
      std::reverse(solucao.rotas[rotaI].begin(), solucao.rotas[rotaI].end());
      std::reverse(solucao.rotas[rotaJ].begin(), solucao.rotas[rotaJ].end());
    }

    solucao.rotas[rotaI].insert(solucao.rotas[rotaI].end(),
        solucao.rotas[rotaJ].begin(), solucao.rotas[rotaJ].end());
    demandaRota[rotaI] += demandaRota[rotaJ];
    solucao.rotas.erase(solucao.rotas.begin() + rotaJ);
    demandaRota.erase(demandaRota.begin() + rotaJ);
  }

  while (instance.num_trucks > 0 &&
         static_cast<int>(solucao.rotas.size()) > instance.num_trucks) {

    auto custoMerge = [&](const std::vector<int>& rA, const std::vector<int>& rB) -> double {
      const Node& ultimo   = instance.getNode(rA.back());
      const Node& primeiro = instance.getNode(rB.front());
      return instance.distancia(ultimo, primeiro)
           - instance.distancia(ultimo, deposito)
           - instance.distancia(deposito, primeiro);
    };

    double melhorCusto = std::numeric_limits<double>::max();
    int melhorI = -1, melhorJ = -1;
    bool melhorRevI = false, melhorRevJ = false;

    for (size_t i = 0; i < solucao.rotas.size(); i++) {
      for (size_t j = i + 1; j < solucao.rotas.size(); j++) {
        if (demandaRota[i] + demandaRota[j] > instance.capacity) continue;
        const auto& rI = solucao.rotas[i];
        const auto& rJ = solucao.rotas[j];
        std::vector<int> rI_rev(rI.rbegin(), rI.rend());
        std::vector<int> rJ_rev(rJ.rbegin(), rJ.rend());
        struct Opt { double c; bool ri; bool rj; };
        Opt opts[4] = {
          { custoMerge(rI,     rJ),     false, false },
          { custoMerge(rI,     rJ_rev), false, true  },
          { custoMerge(rI_rev, rJ),     true,  false },
          { custoMerge(rI_rev, rJ_rev), true,  true  },
        };
        for (auto& o : opts) {
          if (o.c < melhorCusto) {
            melhorCusto = o.c;
            melhorI = (int)i; melhorJ = (int)j;
            melhorRevI = o.ri; melhorRevJ = o.rj;
          }
        }
      }
    }

    if (melhorI != -1) {
      if (melhorRevI) std::reverse(solucao.rotas[melhorI].begin(), solucao.rotas[melhorI].end());
      if (melhorRevJ) std::reverse(solucao.rotas[melhorJ].begin(), solucao.rotas[melhorJ].end());
      solucao.rotas[melhorI].insert(solucao.rotas[melhorI].end(),
          solucao.rotas[melhorJ].begin(), solucao.rotas[melhorJ].end());
      demandaRota[melhorI] += demandaRota[melhorJ];
      solucao.rotas.erase(solucao.rotas.begin() + melhorJ);
      demandaRota.erase(demandaRota.begin() + melhorJ);
      continue;
    }

    std::vector<int> ordemCand(solucao.rotas.size());
    for (size_t k = 0; k < solucao.rotas.size(); k++) ordemCand[k] = (int)k;
    std::sort(ordemCand.begin(), ordemCand.end(),
              [&](int a, int b) { return demandaRota[a] < demandaRota[b]; });

    bool esvaziou = false;
    for (int idxCand : ordemCand) {
      std::vector<int> clientes = solucao.rotas[idxCand];
      std::sort(clientes.begin(), clientes.end(),
                [&](int a, int b) {
                  return instance.getNode(a).demanda > instance.getNode(b).demanda;
                });

      std::vector<int> demAtual = demandaRota;
      demAtual[idxCand] = 0;
      std::vector<std::pair<int,int>> plano(clientes.size(), {-1, -1});
      bool todosOk = true;

      for (size_t ci = 0; ci < clientes.size(); ci++) {
        int cust = clientes[ci];
        int demCust = instance.getNode(cust).demanda;
        const Node& nc = instance.getNode(cust);

        double melhorDelta = std::numeric_limits<double>::max();
        int melhorRota = -1, melhorPos = -1;

        for (size_t k = 0; k < solucao.rotas.size(); k++) {
          if ((int)k == idxCand) continue;
          if (demAtual[k] + demCust > instance.capacity) continue;
          const auto& r = solucao.rotas[k];
          for (size_t pos = 0; pos <= r.size(); pos++) {
            const Node& antes  = (pos == 0)        ? deposito : instance.getNode(r[pos-1]);
            const Node& depois = (pos == r.size()) ? deposito : instance.getNode(r[pos]);
            double delta = instance.distancia(antes, nc)
                         + instance.distancia(nc, depois)
                         - instance.distancia(antes, depois);
            if (delta < melhorDelta) {
              melhorDelta = delta;
              melhorRota = (int)k;
              melhorPos = (int)pos;
            }
          }
        }

        if (melhorRota == -1) { todosOk = false; break; }
        plano[ci] = {melhorRota, melhorPos};
        demAtual[melhorRota] += demCust;
      }

      if (!todosOk) continue;

      std::vector<std::vector<std::pair<int,int>>> porRota(solucao.rotas.size());
      for (size_t ci = 0; ci < clientes.size(); ci++) {
        porRota[plano[ci].first].push_back({plano[ci].second, clientes[ci]});
      }
      for (size_t k = 0; k < porRota.size(); k++) {
        if (porRota[k].empty()) continue;
        std::sort(porRota[k].begin(), porRota[k].end(),
                  [](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                    return a.first > b.first;
                  });
        for (auto& pc : porRota[k]) {
          solucao.rotas[k].insert(solucao.rotas[k].begin() + pc.first, pc.second);
          demandaRota[k] += instance.getNode(pc.second).demanda;
        }
      }
      solucao.rotas.erase(solucao.rotas.begin() + idxCand);
      demandaRota.erase(demandaRota.begin() + idxCand);
      esvaziou = true;
      break;
    }

    if (!esvaziou) {
      std::vector<int> clientes;
      clientes.reserve(instance.nodes.size() - 1);
      for (const auto& rota : solucao.rotas) {
        clientes.insert(clientes.end(), rota.begin(), rota.end());
      }

      std::sort(clientes.begin(), clientes.end(),
                [&](int a, int b) {
                  return instance.getNode(a).demanda > instance.getNode(b).demanda;
                });

      std::vector<std::vector<int>> rotasReempacotadas(instance.num_trucks);
      std::vector<int> demandasReempacotadas(instance.num_trucks, 0);

      if (empacotaPorDemanda(instance, clientes, rotasReempacotadas,
                             demandasReempacotadas, 0)) {
        for (auto& rota : rotasReempacotadas) {
          ordenaRotaPorVizinhoMaisProximo(instance, rota);
        }

        rotasReempacotadas.erase(
            std::remove_if(rotasReempacotadas.begin(), rotasReempacotadas.end(),
                           [](const std::vector<int>& rota) { return rota.empty(); }),
            rotasReempacotadas.end());
        demandasReempacotadas.erase(
            std::remove(demandasReempacotadas.begin(), demandasReempacotadas.end(), 0),
            demandasReempacotadas.end());

        solucao.rotas = rotasReempacotadas;
        demandaRota = demandasReempacotadas;
      }
      break;
    }
  }

  return solucao;
}
