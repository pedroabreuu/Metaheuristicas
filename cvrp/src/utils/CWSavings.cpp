#include <vector>
#include <algorithm>
#include <limits>
#include <utility>
#include <stdexcept>
#include <random>
#include "utils/CWSavings.h"

struct Saving {
  int i;
  int j;
  double valor;
};

struct Insercao {
  size_t rota;
  size_t posicao;
  double delta;
};

static Insercao melhorInsercaoNaRota(
    const VRPInstance& instance,
    const std::vector<int>& rota,
    int cliente) {
  const Node& deposito = instance.getDepot();
  const Node& novo = instance.getNode(cliente);
  Insercao melhor{0, 0, std::numeric_limits<double>::max()};

  for (size_t pos = 0; pos <= rota.size(); pos++) {
    const Node& antes = (pos == 0) ? deposito : instance.getNode(rota[pos - 1]);
    const Node& depois = (pos == rota.size()) ? deposito : instance.getNode(rota[pos]);
    const double delta = instance.distancia(antes, novo)
        + instance.distancia(novo, depois)
        - instance.distancia(antes, depois);
    if (delta < melhor.delta) melhor = {0, pos, delta};
  }

  return melhor;
}

static bool melhorInsercao(
    const VRPInstance& instance,
    const std::vector<std::vector<int>>& rotas,
    const std::vector<int>& demandas,
    int cliente,
    Insercao& melhor) {
  const int demanda = instance.getNode(cliente).demanda;
  melhor = {0, 0, std::numeric_limits<double>::max()};

  for (size_t r = 0; r < rotas.size(); r++) {
    if (demandas[r] + demanda > instance.capacity) continue;
    Insercao candidata = melhorInsercaoNaRota(instance, rotas[r], cliente);
    candidata.rota = r;
    const int folgaCandidata = instance.capacity - demandas[r] - demanda;
    const int folgaMelhor = melhor.delta == std::numeric_limits<double>::max()
        ? std::numeric_limits<int>::max()
        : instance.capacity - demandas[melhor.rota] - demanda;
    if (folgaCandidata < folgaMelhor ||
        (folgaCandidata == folgaMelhor && candidata.delta < melhor.delta)) {
      melhor = candidata;
    }
  }

  return melhor.delta != std::numeric_limits<double>::max();
}

static bool tentaInsercaoPorEjecao(
    const VRPInstance& instance,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas,
    int cliente) {
  const int demandaCliente = instance.getNode(cliente).demanda;
  double melhorDelta = std::numeric_limits<double>::max();
  size_t melhorRota = 0, melhorPosRemocao = 0;
  Insercao melhorDestino{0, 0, 0.0};
  Insercao melhorInsercaoCliente{0, 0, 0.0};

  for (size_t r = 0; r < rotas.size(); r++) {
    for (size_t pos = 0; pos < rotas[r].size(); pos++) {
      const int ejetado = rotas[r][pos];
      const int demandaEjetado = instance.getNode(ejetado).demanda;
      if (demandas[r] - demandaEjetado + demandaCliente > instance.capacity) continue;

      std::vector<int> rotaSemEjetado = rotas[r];
      rotaSemEjetado.erase(rotaSemEjetado.begin() + static_cast<std::ptrdiff_t>(pos));
      Insercao insercaoCliente = melhorInsercaoNaRota(instance, rotaSemEjetado, cliente);

      for (size_t destino = 0; destino < rotas.size(); destino++) {
        if (destino == r || demandas[destino] + demandaEjetado > instance.capacity) continue;
        Insercao insercaoEjetado = melhorInsercaoNaRota(instance, rotas[destino], ejetado);
        const double delta = insercaoCliente.delta + insercaoEjetado.delta;
        if (delta < melhorDelta) {
          melhorDelta = delta;
          melhorRota = r;
          melhorPosRemocao = pos;
          melhorDestino = insercaoEjetado;
          melhorDestino.rota = destino;
          melhorInsercaoCliente = insercaoCliente;
          melhorInsercaoCliente.rota = r;
        }
      }
    }
  }

  if (melhorDelta == std::numeric_limits<double>::max()) return false;

  const int ejetado = rotas[melhorRota][melhorPosRemocao];
  rotas[melhorRota].erase(rotas[melhorRota].begin() + static_cast<std::ptrdiff_t>(melhorPosRemocao));
  rotas[melhorRota].insert(
      rotas[melhorRota].begin() + static_cast<std::ptrdiff_t>(melhorInsercaoCliente.posicao),
      cliente);
  rotas[melhorDestino.rota].insert(
      rotas[melhorDestino.rota].begin() + static_cast<std::ptrdiff_t>(melhorDestino.posicao),
      ejetado);

  demandas[melhorRota] += demandaCliente - instance.getNode(ejetado).demanda;
  demandas[melhorDestino.rota] += instance.getNode(ejetado).demanda;
  return true;
}

static bool eliminaRota(
    const VRPInstance& instance,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas,
    size_t indiceRota) {
  std::vector<int> pendentes = rotas[indiceRota];
  rotas.erase(rotas.begin() + static_cast<std::ptrdiff_t>(indiceRota));
  demandas.erase(demandas.begin() + static_cast<std::ptrdiff_t>(indiceRota));

  while (!pendentes.empty()) {
    size_t indiceEscolhido = pendentes.size();
    Insercao insercaoEscolhida{0, 0, 0.0};
    int maiorDemanda = -1;

    for (size_t i = 0; i < pendentes.size(); i++) {
      Insercao candidata{0, 0, 0.0};
      if (!melhorInsercao(instance, rotas, demandas, pendentes[i], candidata)) {
        continue;
      }

      const int demanda = instance.getNode(pendentes[i]).demanda;
      if (demanda > maiorDemanda) {
        maiorDemanda = demanda;
        indiceEscolhido = i;
        insercaoEscolhida = candidata;
      }
    }

    if (indiceEscolhido < pendentes.size()) {
      const int cliente = pendentes[indiceEscolhido];
      rotas[insercaoEscolhida.rota].insert(
          rotas[insercaoEscolhida.rota].begin() + static_cast<std::ptrdiff_t>(insercaoEscolhida.posicao),
          cliente);
      demandas[insercaoEscolhida.rota] += instance.getNode(cliente).demanda;
      pendentes.erase(pendentes.begin() + static_cast<std::ptrdiff_t>(indiceEscolhido));
      continue;
    }

    auto clienteMaisDificil = std::max_element(
        pendentes.begin(), pendentes.end(),
        [&](int a, int b) { return instance.getNode(a).demanda < instance.getNode(b).demanda; });
    if (!tentaInsercaoPorEjecao(instance, rotas, demandas, *clienteMaisDificil)) return false;
    pendentes.erase(clienteMaisDificil);
  }

  return true;
}

static bool eliminaUmaRota(
    const VRPInstance& instance,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas) {
  std::vector<size_t> candidatas(rotas.size());
  for (size_t i = 0; i < candidatas.size(); i++) candidatas[i] = i;
  std::sort(candidatas.begin(), candidatas.end(),
      [&](size_t a, size_t b) { return demandas[a] < demandas[b]; });

  for (size_t candidata : candidatas) {
    auto rotasTeste = rotas;
    auto demandasTeste = demandas;
    if (eliminaRota(instance, rotasTeste, demandasTeste, candidata)) {
      rotas = std::move(rotasTeste);
      demandas = std::move(demandasTeste);
      return true;
    }
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
    int melhorDistancia = std::numeric_limits<int>::max();
    for (auto it = naoVisitados.begin(); it != naoVisitados.end(); ++it) {
      const int distancia = instance.distancia(*atual, instance.getNode(*it));
      if (distancia < melhorDistancia) {
        melhorDistancia = distancia;
        melhor = it;
      }
    }
    rota.push_back(*melhor);
    atual = &instance.getNode(*melhor);
    naoVisitados.erase(melhor);
  }
}

static long long sobrecargaQuadratica(int carga, int capacidade) {
  const long long excesso = std::max(0, carga - capacidade);
  return excesso * excesso;
}

static bool balanceiaDemandas(
    const VRPInstance& instance,
    const std::vector<int>& clientes,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas) {
  std::mt19937 gen(0xBADC0DEu);
  constexpr int reinicios = 128;

  for (int reinicio = 0; reinicio < reinicios; reinicio++) {
    std::vector<int> ordem = clientes;
    std::shuffle(ordem.begin(), ordem.end(), gen);
    std::stable_sort(ordem.begin(), ordem.end(), [&](int a, int b) {
      return instance.getNode(a).demanda > instance.getNode(b).demanda;
    });

    std::vector<std::vector<int>> candidatas(static_cast<size_t>(instance.num_trucks));
    std::vector<int> cargas(static_cast<size_t>(instance.num_trucks), 0);
    for (int cliente : ordem) {
      const auto destino = std::min_element(cargas.begin(), cargas.end());
      const size_t rota = static_cast<size_t>(std::distance(cargas.begin(), destino));
      candidatas[rota].push_back(cliente);
      cargas[rota] += instance.getNode(cliente).demanda;
    }

    long long custo = 0;
    for (int carga : cargas) custo += sobrecargaQuadratica(carga, instance.capacity);
    if (custo == 0) {
      rotas = std::move(candidatas);
      demandas = std::move(cargas);
      return true;
    }

    constexpr int iteracoes = 150000;
    for (int iteracao = 0; iteracao < iteracoes; iteracao++) {
      size_t origem = 0;
      for (size_t r = 1; r < cargas.size(); r++) {
        if (cargas[r] > cargas[origem]) origem = r;
      }
      if (cargas[origem] <= instance.capacity || candidatas[origem].empty()) break;

      std::uniform_int_distribution<size_t> escolheOrigem(0, candidatas[origem].size() - 1);
      std::uniform_int_distribution<size_t> escolheRota(0, candidatas.size() - 1);
      const size_t posOrigem = escolheOrigem(gen);
      size_t destino = escolheRota(gen);
      if (destino == origem || candidatas[destino].empty()) continue;
      std::uniform_int_distribution<size_t> escolheDestino(0, candidatas[destino].size() - 1);
      const size_t posDestino = escolheDestino(gen);

      const int clienteOrigem = candidatas[origem][posOrigem];
      const int clienteDestino = candidatas[destino][posDestino];
      const int novaCargaOrigem = cargas[origem]
          - instance.getNode(clienteOrigem).demanda + instance.getNode(clienteDestino).demanda;
      const int novaCargaDestino = cargas[destino]
          - instance.getNode(clienteDestino).demanda + instance.getNode(clienteOrigem).demanda;
      const long long novoCusto = custo
          - sobrecargaQuadratica(cargas[origem], instance.capacity)
          - sobrecargaQuadratica(cargas[destino], instance.capacity)
          + sobrecargaQuadratica(novaCargaOrigem, instance.capacity)
          + sobrecargaQuadratica(novaCargaDestino, instance.capacity);

      const bool melhora = novoCusto < custo;
      const bool aceitaEmpate = novoCusto == custo && (gen() % 8U) == 0U;
      const bool aceitaEscape = novoCusto > custo && (gen() % 2000U) == 0U;
      if (!melhora && !aceitaEmpate && !aceitaEscape) continue;

      std::swap(candidatas[origem][posOrigem], candidatas[destino][posDestino]);
      cargas[origem] = novaCargaOrigem;
      cargas[destino] = novaCargaDestino;
      custo = novoCusto;
      if (custo == 0) {
        rotas = std::move(candidatas);
        demandas = std::move(cargas);
        return true;
      }
    }
  }

  return false;
}

static bool reempacotaEmNumeroFixoDeRotas(
    const VRPInstance& instance,
    const std::vector<std::vector<int>>& rotasOriginais,
    std::vector<std::vector<int>>& rotas,
    std::vector<int>& demandas) {
  std::vector<int> clientes;
  for (const auto& rota : rotasOriginais) {
    clientes.insert(clientes.end(), rota.begin(), rota.end());
  }

  if (balanceiaDemandas(instance, clientes, rotas, demandas)) {
    for (auto& rota : rotas) ordenaRotaPorVizinhoMaisProximo(instance, rota);
    return true;
  }

  std::sort(clientes.begin(), clientes.end(), [&](int a, int b) {
    return instance.getNode(a).demanda > instance.getNode(b).demanda;
  });

  std::mt19937 gen(0xC0FFEEu);
  constexpr int tentativas = 128;
  for (int tentativa = 0; tentativa < tentativas; tentativa++) {
    std::vector<int> ordem = clientes;
    if (tentativa > 0) {
      std::shuffle(ordem.begin(), ordem.end(), gen);
      std::stable_sort(ordem.begin(), ordem.end(), [&](int a, int b) {
        return instance.getNode(a).demanda > instance.getNode(b).demanda;
      });
    }

    std::vector<std::vector<int>> candidatas(static_cast<size_t>(instance.num_trucks));
    std::vector<int> demandasCandidatas(static_cast<size_t>(instance.num_trucks), 0);
    bool viavel = true;

    for (int cliente : ordem) {
      const int demanda = instance.getNode(cliente).demanda;
      int melhorRota = -1;
      int menorFolga = std::numeric_limits<int>::max();
      for (size_t r = 0; r < candidatas.size(); r++) {
        const int folga = instance.capacity - demandasCandidatas[r] - demanda;
        if (folga < 0 || folga > menorFolga) continue;
        if (folga < menorFolga || (gen() & 1U) == 0U) {
          menorFolga = folga;
          melhorRota = static_cast<int>(r);
        }
      }

      if (melhorRota == -1) {
        viavel = false;
        break;
      }
      candidatas[static_cast<size_t>(melhorRota)].push_back(cliente);
      demandasCandidatas[static_cast<size_t>(melhorRota)] += demanda;
    }

    if (!viavel) continue;
    for (auto& rota : candidatas) ordenaRotaPorVizinhoMaisProximo(instance, rota);
    candidatas.erase(std::remove_if(candidatas.begin(), candidatas.end(),
        [](const std::vector<int>& rota) { return rota.empty(); }), candidatas.end());
    demandasCandidatas.erase(std::remove(demandasCandidatas.begin(), demandasCandidatas.end(), 0),
        demandasCandidatas.end());
    rotas = std::move(candidatas);
    demandas = std::move(demandasCandidatas);
    return true;
  }

  return false;
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
      const auto rotasAntesDoReparo = solucao.rotas;
      if (!eliminaUmaRota(instance, solucao.rotas, demandaRota) &&
          !reempacotaEmNumeroFixoDeRotas(
              instance, rotasAntesDoReparo, solucao.rotas, demandaRota)) {
        throw std::runtime_error(
            "Clarke-Wright nao encontrou uma solucao dentro do limite de veiculos");
      }
    }
  }

  return solucao;
}
