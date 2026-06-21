#include "utils/experimento.h"
#include "utils/rng.h"
#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <limits>
#include <utility>
#include "utils/CWSavings.h"
#include "metaheuristicas/ils.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

using NeighborhoodFunc = Solution (*)(Solution, const VRPInstance&);

enum class BuscaLocal {
    VND,
    RVND
};

enum class TipoPerturbacao {
    Relocate,
    Opt2,
    CrossExchange,
    SwapIntra,
    SwapInter,
    OrOptInter,
    Opt2Star
};

struct ResultadoPerturbacao {
    Solution solucao;
    std::vector<int> operadores;
};

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

static Solution buscaLocal(Solution solucao, const VRPInstance& instance, std::mt19937& gen, BuscaLocal modo) {
    limpaRotasVazias(solucao);
    std::vector<NeighborhoodFunc> vizinhancas = {
        opt2,
        relocate,
        orOptIntra3,
        orOptInter3,
        swapIntra,
        swapInter,
        crossExchange,
        opt2Star
    };

    if (modo == BuscaLocal::RVND) {
        std::shuffle(vizinhancas.begin(), vizinhancas.end(), gen);
    }

    solucao.calculaCusto(instance);
    int k = 0;

    while (k < static_cast<int>(vizinhancas.size())) {
        Solution candidata = vizinhancas[k](std::move(Solution(solucao)), instance);
        candidata.calculaCusto(instance);

        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = std::move(candidata);
            k = 0;
            if (modo == BuscaLocal::RVND) {
                std::shuffle(vizinhancas.begin(), vizinhancas.end(), gen);
            }
        } else {
            k++;
        }
    }

    return solucao;
}

static Solution aplicaPerturbacao(
    Solution solucao,
    const VRPInstance& instance,
    TipoPerturbacao tipo
) {
    switch (tipo) {
        case TipoPerturbacao::Relocate:
            return randomRelocate(std::move(solucao), instance);
        case TipoPerturbacao::Opt2:
            return randomOpt2(std::move(solucao), instance);
        case TipoPerturbacao::CrossExchange:
            return randomCrossExchange(std::move(solucao), instance);
        case TipoPerturbacao::SwapIntra:
            return randomSwapIntra(std::move(solucao), instance);
        case TipoPerturbacao::SwapInter:
            return randomSwapInter(std::move(solucao), instance);
        case TipoPerturbacao::OrOptInter:
            return randomOrOptInter(std::move(solucao), instance, 3);
        case TipoPerturbacao::Opt2Star:
            return randomOpt2Star(std::move(solucao), instance);
    }

    return solucao;
}

static ResultadoPerturbacao perturbar(
    Solution solucao,
    const VRPInstance& instance,
    int intensidade,
    const std::vector<double>& pesosOperadores,
    std::mt19937& gen
) {
    std::vector<TipoPerturbacao> perturbacoes = {
        TipoPerturbacao::Relocate,
        TipoPerturbacao::Opt2,
        TipoPerturbacao::CrossExchange,
        TipoPerturbacao::SwapIntra,
        TipoPerturbacao::SwapInter,
        TipoPerturbacao::OrOptInter,
        TipoPerturbacao::Opt2Star
    };

    std::discrete_distribution<int> perturbacao(pesosOperadores.begin(), pesosOperadores.end());

    intensidade = std::max(1, intensidade);
    ResultadoPerturbacao resultado{std::move(solucao), {}};
    resultado.operadores.reserve(static_cast<size_t>(intensidade));

    for (int i = 0; i < intensidade; i++) {
        int indicePerturbacao = perturbacao(gen);
        resultado.solucao = aplicaPerturbacao(
            std::move(resultado.solucao),
            instance,
            perturbacoes[static_cast<size_t>(indicePerturbacao)]
        );
        resultado.operadores.push_back(indicePerturbacao);
    }

    limpaRotasVazias(resultado.solucao);
    return resultado;
}

static Solution perturbarForte(
    Solution solucao,
    const VRPInstance& instance,
    int intensidadeMaxima,
    std::mt19937& gen
) {
    std::vector<TipoPerturbacao> perturbacoes = {
        TipoPerturbacao::Relocate,
        TipoPerturbacao::Opt2,
        TipoPerturbacao::CrossExchange,
        TipoPerturbacao::SwapIntra,
        TipoPerturbacao::SwapInter,
        TipoPerturbacao::OrOptInter,
        TipoPerturbacao::Opt2Star
    };

    std::shuffle(perturbacoes.begin(), perturbacoes.end(), gen);

    intensidadeMaxima = std::max(1, intensidadeMaxima);
    int quantidade = std::min(intensidadeMaxima, static_cast<int>(perturbacoes.size()));

    for (int i = 0; i < quantidade; i++) {
        solucao = aplicaPerturbacao(std::move(solucao), instance, perturbacoes[static_cast<size_t>(i)]);
    }

    limpaRotasVazias(solucao);
    return solucao;
}

static Solution constroiAleatoria(const VRPInstance& instance, std::mt19937& gen) {
    struct Insercao {
        size_t rota;
        size_t posicao;
        double delta;
        bool novaRota;
    };

    Solution solucao;
    std::vector<int> cargas;
    std::vector<int> clientes;
    clientes.reserve(instance.nodes.size());

    for (const Node& node : instance.nodes) {
        if (node.id != instance.depot_id) {
            clientes.push_back(node.id);
        }
    }

    std::shuffle(clientes.begin(), clientes.end(), gen);
    const Node& depot = instance.getDepot();

    for (int clienteId : clientes) {
        const Node& cliente = instance.getNode(clienteId);
        std::vector<Insercao> candidatos;

        for (size_t r = 0; r < solucao.rotas.size(); r++) {
            if (cargas[r] + cliente.demanda > instance.capacity) continue;

            for (size_t pos = 0; pos <= solucao.rotas[r].size(); pos++) {
                const Node& antes = (pos == 0) ? depot : instance.getNode(solucao.rotas[r][pos - 1]);
                const Node& depois = (pos == solucao.rotas[r].size()) ? depot : instance.getNode(solucao.rotas[r][pos]);
                double delta = instance.distancia(antes, cliente)
                             + instance.distancia(cliente, depois)
                             - instance.distancia(antes, depois);
                candidatos.push_back({r, pos, delta, false});
            }
        }

        if (instance.num_trucks == 0 || static_cast<int>(solucao.rotas.size()) < instance.num_trucks) {
            double delta = instance.distancia(depot, cliente) + instance.distancia(cliente, depot);
            candidatos.push_back({solucao.rotas.size(), 0, delta, true});
        }

        if (candidatos.empty()) {
            solucao.rotas.push_back({clienteId});
            cargas.push_back(cliente.demanda);
            continue;
        }

        std::sort(candidatos.begin(), candidatos.end(),
            [](const Insercao& a, const Insercao& b) { return a.delta < b.delta; });

        int limiteRcl = std::min(3, static_cast<int>(candidatos.size()));
        std::uniform_int_distribution<int> escolha(0, limiteRcl - 1);
        const Insercao escolhida = candidatos[static_cast<size_t>(escolha(gen))];

        if (escolhida.novaRota) {
            solucao.rotas.push_back({clienteId});
            cargas.push_back(cliente.demanda);
        } else {
            solucao.rotas[escolhida.rota].insert(
                solucao.rotas[escolhida.rota].begin() + static_cast<long>(escolhida.posicao),
                clienteId
            );
            cargas[escolhida.rota] += cliente.demanda;
        }
    }

    limpaRotasVazias(solucao);
    solucao.calculaCusto(instance);
    return solucao;
}

static void recompensaOperadores(std::vector<double>& pesosOperadores, const std::vector<int>& operadores) {
    for (int operador : operadores) {
        pesosOperadores[static_cast<size_t>(operador)] += 1.0;
    }

    double soma = std::accumulate(pesosOperadores.begin(), pesosOperadores.end(), 0.0);
    if (soma > 50.0) {
        for (double& peso : pesosOperadores) {
            peso = std::max(0.20, peso / soma * static_cast<double>(pesosOperadores.size()));
        }
    }
}

Solution ILS(
    const VRPInstance& instance,
    double tempoLimite,
    int iterSemMelhoraMax,
    int Bmin,
    int Bmax,
    int BmaxStuck,
    bool usarRVND
) {
    const BuscaLocal modo = usarRVND ? BuscaLocal::RVND : BuscaLocal::VND;
    std::mt19937& gen = rngGlobal();
    std::uniform_real_distribution<> dist(0.0, 1.0);

    Bmin = std::max(1, Bmin);
    Bmax = std::max(Bmin, Bmax);
    BmaxStuck = std::max(Bmax, BmaxStuck);
    const int baseBmin = Bmin;
    const int baseBmax = Bmax;
    int bMinAtual = Bmin;
    int bMaxAtual = Bmax;

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    Solution s0 = clarkeWright(instance);
    Solution s = buscaLocal(std::move(s0), instance, gen, modo);
    Solution best = s;
    expRegistraMelhora(best.custoTotal);
 
    const double temperaturaInicial = std::max(1.0, 0.05 * static_cast<double>(best.custoTotal));
    double temperatura = temperaturaInicial;
    int iterSemMelhora = 0;
    int iteracao = 0;
    std::vector<double> pesosOperadores(7, 1.0);

    std::cout << "ILS inicio | Custo: " << best.custoTotal << std::endl;

    if (best.custoTotal == instance.optimal_value) {
        std::cout << "ILS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
        return best;
    }

    while (true) {
        auto agora = std::chrono::steady_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        if (tempoDecorrido >= tempoLimite) break;

        std::uniform_int_distribution<> betaDist(bMinAtual, bMaxAtual);
        int beta = betaDist(gen);
        ResultadoPerturbacao perturbada = perturbar(s, instance, beta, pesosOperadores, gen);
        Solution candidata = buscaLocal(std::move(perturbada.solucao), instance, gen, modo);

        double delta = candidata.custoTotal - s.custoTotal;
        bool melhorouBest = false;

        if (delta < 0) {
            s = std::move(candidata);

            if (s.custoTotal < best.custoTotal) {
                best = s;
                expRegistraMelhora(best.custoTotal);
                melhorouBest = true;
                recompensaOperadores(pesosOperadores, perturbada.operadores);
                tempoBest = std::chrono::steady_clock::now();
                std::cout << "ILS iter " << iteracao << " | Melhor: " << best.custoTotal << " em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;

                if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
                    std::cout << "ILS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
                    return best;
                }
            }
        } else if (temperatura > 0 && dist(gen) < std::exp(-delta / temperatura)) {
            s = std::move(candidata);
        }

        if (melhorouBest) {
            iterSemMelhora = 0;
            bMinAtual = baseBmin;
            bMaxAtual = baseBmax;
        } else {
            iterSemMelhora++;
            if (iterSemMelhora % std::max(5, iterSemMelhoraMax / 4) == 0) {
                bMaxAtual = std::min(BmaxStuck, bMaxAtual + 1);
                if (bMaxAtual > baseBmax + 1) {
                    bMinAtual = std::min(bMaxAtual, bMinAtual + 1);
                }
            }
        }

        agora = std::chrono::steady_clock::now();
        tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        double progresso = (tempoLimite > 0.0) ? std::min(1.0, tempoDecorrido / tempoLimite) : 1.0;
        double alphaDinamico = 0.9999 - (0.0049 * progresso);
        temperatura = std::max(temperaturaInicial * 0.001, temperatura * alphaDinamico);

        if (iterSemMelhora >= iterSemMelhoraMax) {
            Solution reconstruida = constroiAleatoria(instance, gen);
            reconstruida = buscaLocal(std::move(reconstruida), instance, gen, modo);

            Solution escape = perturbarForte(best, instance, BmaxStuck, gen);
            escape = buscaLocal(std::move(escape), instance, gen, modo);

            s = (reconstruida.custoTotal < escape.custoTotal)
                ? std::move(reconstruida)
                : std::move(escape);

            temperatura = temperaturaInicial;
            iterSemMelhora = 0;
            bMinAtual = baseBmin;
            bMaxAtual = baseBmax;
        }

        iteracao++;
    }

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    std::cout << "ILS: Melhor=" << best.custoTotal << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

    return best;
}
