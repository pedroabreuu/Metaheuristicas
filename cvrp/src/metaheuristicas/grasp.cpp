#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <limits>
#include <chrono>
#include <cmath>
#include <utility>
#include "metaheuristicas/grasp.h"
#include "metaheuristicas/vns.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

using NeighborhoodFunc = Solution (*)(Solution, const VRPInstance&);

struct InsertionCandidate {
    int cliente;
    int rota;
    int posicao;
    double delta;
    bool novaRota;
};

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

static double calculaDeltaInsercao(const std::vector<int>& rota, int posicao, int cliente, const VRPInstance& instance) {
    if (posicao < 0 || posicao > static_cast<int>(rota.size())) {
        return std::numeric_limits<double>::infinity();
    }

    const Node& anterior = (posicao == 0)
        ? instance.getDepot()
        : instance.getNode(rota[posicao - 1]);
    const Node& proximo = (posicao == static_cast<int>(rota.size()))
        ? instance.getDepot()
        : instance.getNode(rota[posicao]);

    return instance.distancia(anterior, instance.getNode(cliente))
         + instance.distancia(instance.getNode(cliente), proximo)
         - instance.distancia(anterior, proximo);
}

static bool insercaoViavel(int cargaAtual, int cliente, const VRPInstance& instance) {
    return cargaAtual + instance.getNode(cliente).demanda <= instance.capacity;
}

static std::vector<InsertionCandidate> gerarCandidatos(
    const Solution& solucao,
    const std::vector<int>& naoInseridos,
    const std::vector<int>& cargasRotas,
    const VRPInstance& instance
) {
    std::vector<InsertionCandidate> candidatos;
    size_t totalPosicoes = 0;
    for (const auto& rota : solucao.rotas) {
        totalPosicoes += rota.size() + 1;
    }
    candidatos.reserve(naoInseridos.size() * std::max<size_t>(1, totalPosicoes));

    for (int cliente : naoInseridos) {
        bool temInsercaoViavel = false;

        for (int j = 0; j < static_cast<int>(solucao.rotas.size()); j++) {
            if (!insercaoViavel(cargasRotas[j], cliente, instance)) {
                continue;
            }

            for (int posicao = 0; posicao <= static_cast<int>(solucao.rotas[j].size()); posicao++) {
                double delta = calculaDeltaInsercao(solucao.rotas[j], posicao, cliente, instance);

                if (std::isfinite(delta)) {
                    InsertionCandidate candidato;
                    candidato.cliente = cliente;
                    candidato.rota = j;
                    candidato.posicao = posicao;
                    candidato.delta = delta;
                    candidato.novaRota = false;
                    candidatos.push_back(candidato);
                    temInsercaoViavel = true;
                }
            }
        }

        if (!temInsercaoViavel && instance.getNode(cliente).demanda <= instance.capacity) {
            double deltaNovaRota =
                instance.distancia(instance.getDepot(), instance.getNode(cliente)) +
                instance.distancia(instance.getNode(cliente), instance.getDepot());

            InsertionCandidate candidatoNovaRota;
            candidatoNovaRota.cliente = cliente;
            candidatoNovaRota.rota = -1;
            candidatoNovaRota.posicao = -1;
            candidatoNovaRota.delta = deltaNovaRota;
            candidatoNovaRota.novaRota = true;
            candidatos.push_back(candidatoNovaRota);
        }
    }

    return candidatos;
}

static std::vector<InsertionCandidate> montarRCL(const std::vector<InsertionCandidate>& candidatos, double alpha) {
    std::vector<InsertionCandidate> rcl;

    if (candidatos.empty()) {
        return rcl;
    }

    rcl.reserve(candidatos.size());

    double deltaMin = candidatos.front().delta;
    double deltaMax = candidatos.front().delta;

    for (const auto& candidato : candidatos) {
        deltaMin = std::min(deltaMin, candidato.delta);
        deltaMax = std::max(deltaMax, candidato.delta);
    }

    double limite = deltaMin + alpha * (deltaMax - deltaMin);

    for (const auto& candidato : candidatos) {
        if (candidato.delta <= limite) {
            rcl.push_back(candidato);
        }
    }

    return rcl;
}

static InsertionCandidate escolheCandidatoRCL(const std::vector<InsertionCandidate>& rcl, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(rcl.size()) - 1);
    return rcl[dist(gen)];
}

static void aplicarInsercao(
    Solution& solucao,
    std::vector<int>& cargasRotas,
    const InsertionCandidate& candidato,
    const VRPInstance& instance
) {
    int demandaCliente = instance.getNode(candidato.cliente).demanda;

    if (candidato.novaRota) {
        std::vector<int> novaRota = { candidato.cliente };
        solucao.rotas.push_back(novaRota);
        cargasRotas.push_back(demandaCliente);
    } else {
        solucao.rotas[candidato.rota].insert(
            solucao.rotas[candidato.rota].begin() + candidato.posicao,
            candidato.cliente
        );
        cargasRotas[candidato.rota] += demandaCliente;
    }
}

static Solution constroiGreedyRandomized(const VRPInstance& instance, double alpha, std::mt19937& gen) {
    Solution solucao;
    std::vector<int> naoInseridos;
    std::vector<int> cargasRotas;

    for (const auto& node : instance.nodes) {
        if (node.id != instance.depot_id) {
            naoInseridos.push_back(node.id);
        }
    }

    while (!naoInseridos.empty()) {
        std::vector<InsertionCandidate> candidatos =
            gerarCandidatos(solucao, naoInseridos, cargasRotas, instance);

        if (candidatos.empty()) {
            break;
        }

        std::vector<InsertionCandidate> rcl = montarRCL(candidatos, alpha);

        if (rcl.empty()) {
            break;
        }

        InsertionCandidate escolhido = escolheCandidatoRCL(rcl, gen);
        aplicarInsercao(solucao, cargasRotas, escolhido, instance);

        naoInseridos.erase(
            std::remove(naoInseridos.begin(), naoInseridos.end(), escolhido.cliente),
            naoInseridos.end()
        );
    }

    solucao.calculaCusto(instance);
    return solucao;
}

static Solution RVND(Solution solucao, const VRPInstance& instance, std::mt19937& gen) {
    limpaRotasVazias(solucao);
    std::vector<NeighborhoodFunc> vizinhancas = {opt2, relocate, swapIntra, crossExchange, swapInter};

    solucao.calculaCusto(instance);
    std::vector<int> indices(vizinhancas.size());
    for (size_t i = 0; i < indices.size(); i++) {
        indices[i] = static_cast<int>(i);
    }

    while (!indices.empty()) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(indices.size()) - 1);
        int pos = dist(gen);
        int k = indices[pos];

        limpaRotasVazias(solucao);
        Solution candidata = vizinhancas[k](solucao, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);

        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = candidata;
            indices.clear();
            for (size_t i = 0; i < vizinhancas.size(); i++) {
                indices.push_back(static_cast<int>(i));
            }
        } else {
            indices.erase(indices.begin() + pos);
        }
    }

    return solucao;
}

static Solution aplicarPerturbacao(const Solution& solucao, const VRPInstance& instance,
                                    int intensidade, std::mt19937& gen) {
    Solution perturbada = solucao;
    std::vector<NeighborhoodFunc> perturbacoes = {randomRelocate, randomOpt2, randomCrossExchange, randomSwapInter, randomSwapIntra};
    std::uniform_int_distribution<int> distTipo(0, static_cast<int>(perturbacoes.size()) - 1);

    for (int i = 0; i < intensidade; i++) {
        int tipo = distTipo(gen);
        perturbada = perturbacoes[tipo](std::move(perturbada), instance);
        limpaRotasVazias(perturbada);
    }

    perturbada.calculaCusto(instance);
    return perturbada;
}

Solution GRASP(const VRPInstance& instance, double tempoLimiteSegundos) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distReal(0.0, 1.0);

    const double ALPHA_MIN = 0.05;
    const double ALPHA_MAX = 0.4;
    const double ALPHA_PERTURBADO = 0.55;
    std::uniform_real_distribution<double> distAlpha(ALPHA_MIN, ALPHA_MAX);

    int iteracoesSemMelhora = 0;
    const int LIMITE_ESTAGNACAO = 150;
    const int INTENSIDADE_INICIAL = 1;
    const int INTENSIDADE_MAX = 10;
    int intensidadePerturbacao = INTENSIDADE_INICIAL;

    const double SA_TEMP_INICIAL = 200.0;
    const double SA_ALPHA = 0.9995;
    const double SA_TEMP_MIN = 0.1;
    double temperatura = SA_TEMP_INICIAL;

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    Solution best;
    Solution atual;
    bool primeira = true;
    int iteracao = 0;

    while (true) {
        auto agora = std::chrono::steady_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        if (tempoDecorrido >= tempoLimiteSegundos) {
            break;
        }

        Solution s;
        double alpha;

        if (iteracoesSemMelhora >= LIMITE_ESTAGNACAO && !primeira) {
            Solution sPerturbada = aplicarPerturbacao(atual, instance, intensidadePerturbacao, gen);
            sPerturbada = RVND(sPerturbada, instance, gen);
            sPerturbada.calculaCusto(instance);

            Solution sReconstruida = constroiGreedyRandomized(instance, ALPHA_PERTURBADO, gen);
            sReconstruida = RVND(sReconstruida, instance, gen);
            sReconstruida.calculaCusto(instance);

            s = (sReconstruida.getCusto() < sPerturbada.getCusto()) ? sReconstruida : sPerturbada;

            alpha = ALPHA_PERTURBADO;
            intensidadePerturbacao = std::min(intensidadePerturbacao + 1, INTENSIDADE_MAX);
            iteracoesSemMelhora = 0;

            temperatura = SA_TEMP_INICIAL;

            std::cout << "GRASP perturbacao/reconstrucao (intensidade=" << intensidadePerturbacao - 1
                      << ", alpha=" << alpha
                      << ") | Custo: " << s.getCusto() << std::endl;
        } else {
            alpha = distAlpha(gen);
            s = constroiGreedyRandomized(instance, alpha, gen);
            s = RVND(s, instance, gen);
            s.calculaCusto(instance);
        }

        if (primeira || s.getCusto() < best.getCusto()) {
            best = s;
            atual = s;
            tempoBest = std::chrono::steady_clock::now();
            iteracoesSemMelhora = 0;
            intensidadePerturbacao = INTENSIDADE_INICIAL;

            if (primeira) {
                std::cout << "GRASP inicio | Custo: " << best.getCusto() << std::endl;
            } else {
                std::cout << "GRASP iter " << iteracao
                          << " | Melhor: " << best.getCusto()
                          << " em "
                          << std::chrono::duration<double>(tempoBest - inicio).count()
                          << "s" << std::endl;
            }

            primeira = false;

            if (instance.optimal_value > 0 && best.getCusto() == instance.optimal_value) {
                std::cout << "GRASP: Otimo encontrado em "
                          << std::chrono::duration<double>(tempoBest - inicio).count()
                          << "s" << std::endl;
                return best;
            }
        } else {
            double delta = s.getCusto() - atual.getCusto();

            if (delta < 0) {
                atual = s;
                iteracoesSemMelhora = 0;
            } else if (temperatura > SA_TEMP_MIN && distReal(gen) < std::exp(-delta / temperatura)) {
                atual = s;
                iteracoesSemMelhora++;
            } else {
                iteracoesSemMelhora++;
            }
        }

        temperatura = std::max(temperatura * SA_ALPHA, SA_TEMP_MIN);

        iteracao++;
    }

    auto fim = std::chrono::steady_clock::now();
    double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
    double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

    std::cout << "GRASP: Melhor=" << best.getCusto()
              << " em " << tempoMelhor
              << "s (total: " << tempoTotal << "s)" << std::endl;

    return best;
}
