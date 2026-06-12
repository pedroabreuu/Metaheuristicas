#include "utils/experimento.h"
#include "utils/rng.h"
#include "metaheuristicas/lns.h"
#include "metaheuristicas/vns.h"
#include "core/Solution.h"
#include "core/VRPInstance.h"
#include "utils/CWSavings.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <functional>

struct ResultadoDestruicao {
    Solution solucao;
    std::vector<int> clientes;
};

struct Construcao {
    int melhorRota;
    int melhorPosicao;
    double delta;
};

using FuncDestruicao = std::function<ResultadoDestruicao(const Solution&, const VRPInstance&, double, double, std::mt19937&)>;
using FuncConstrucao = std::function<Solution(const VRPInstance&, const Solution&, const std::vector<int>&)>;

static Solution construirGuloso(const VRPInstance& instance, const Solution& solucao, const std::vector<int>& clientesRemovidos) {
    Solution copiaSolucao = solucao;
    Construcao resultado;
    const Node& depot = instance.getDepot();
    
    for (int i = 0; i < static_cast<int>(clientesRemovidos.size()); ++i) {
        resultado.melhorRota = -1;
        resultado.melhorPosicao = -1;
        double melhorDelta = 1e18;

        for (int j = 0; j < static_cast<int>(copiaSolucao.rotas.size()); ++j) {
            int cargaRota = 0;
            for (int id : copiaSolucao.rotas[static_cast<size_t>(j)]) {
                cargaRota += instance.getNode(id).demanda;
            }

            const Node& cliente = instance.getNode(clientesRemovidos[static_cast<size_t>(i)]);

            if (cargaRota + cliente.demanda > instance.capacity) continue;

            for (int r = 0; r <= static_cast<int>(copiaSolucao.rotas[static_cast<size_t>(j)].size()); ++r) {
                const Node& antes = (r == 0) ? depot : instance.getNode(copiaSolucao.rotas[static_cast<size_t>(j)][static_cast<size_t>(r) - 1]);
                const Node& depois = (r == static_cast<int>(copiaSolucao.rotas[static_cast<size_t>(j)].size())) ? depot : instance.getNode(copiaSolucao.rotas[static_cast<size_t>(j)][static_cast<size_t>(r)]);
                double delta = instance.distancia(antes, cliente) + instance.distancia(cliente, depois)
                             - instance.distancia(antes, depois);

                if (delta < melhorDelta) {
                    melhorDelta = delta;
                    resultado.melhorRota = j;
                    resultado.melhorPosicao = r;
                }
            }
        }

        int clienteId = clientesRemovidos[static_cast<size_t>(i)];
        if (resultado.melhorRota == -1) {
            copiaSolucao.rotas.push_back({clienteId});
        } else {
            copiaSolucao.rotas[static_cast<size_t>(resultado.melhorRota)].insert(
                copiaSolucao.rotas[static_cast<size_t>(resultado.melhorRota)].begin() + resultado.melhorPosicao,
                clienteId
            );
        }
    }

    return copiaSolucao;
}

static Solution construirRegret2(const VRPInstance& instance, const Solution& solucao, const std::vector<int>& clientesRemovidosIniciais) {
    Solution copiaSolucao = solucao;
    std::vector<int> naoInseridos = clientesRemovidosIniciais;
    const Node& depot = instance.getDepot();

    while (!naoInseridos.empty()) {
        double maxRegret = -1.0;
        int melhorClienteIdx = -1;
        int rotaVencedora = -1;
        int posicaoVencedora = -1;

        for (size_t i = 0; i < naoInseridos.size(); ++i) {
            int clienteId = naoInseridos[i];
            const Node& cliente = instance.getNode(clienteId);

            double melhorDelta = 1e18;
            double segundoMelhorDelta = 1e18;
            int rotaTemp = -1;
            int posicaoTemp = -1;

            for (size_t j = 0; j < copiaSolucao.rotas.size(); ++j) {
                int cargaRota = 0;
                for (int id : copiaSolucao.rotas[j]) {
                    cargaRota += instance.getNode(id).demanda;
                }

                if (cargaRota + cliente.demanda > instance.capacity) continue;

                for (size_t r = 0; r <= copiaSolucao.rotas[j].size(); ++r) {
                    const Node& antes = (r == 0) ? depot : instance.getNode(copiaSolucao.rotas[j][r - 1]);
                    const Node& depois = (r == copiaSolucao.rotas[j].size()) ? depot : instance.getNode(copiaSolucao.rotas[j][r]);
                    
                    double delta = instance.distancia(antes, cliente) + instance.distancia(cliente, depois) - instance.distancia(antes, depois);

                    if (delta < melhorDelta) {
                        segundoMelhorDelta = melhorDelta;
                        melhorDelta = delta;
                        rotaTemp = static_cast<int>(j);
                        posicaoTemp = static_cast<int>(r);
                    } else if (delta < segundoMelhorDelta) {
                        segundoMelhorDelta = delta;
                    }
                }
            }

            if (rotaTemp == -1) {
                double custoNovaRota = instance.distancia(depot, cliente) + instance.distancia(cliente, depot);
                melhorDelta = custoNovaRota;
                segundoMelhorDelta = melhorDelta + 1e6; // forca insercao
            } else if (segundoMelhorDelta == 1e18) {
                double custoNovaRota = instance.distancia(depot, cliente) + instance.distancia(cliente, depot);
                segundoMelhorDelta = custoNovaRota;
            }

            double regret = segundoMelhorDelta - melhorDelta;

            if (regret > maxRegret) {
                maxRegret = regret;
                melhorClienteIdx = static_cast<int>(i);
                rotaVencedora = rotaTemp;
                posicaoVencedora = posicaoTemp;
            }
        }

        int clienteVencedorId = naoInseridos[melhorClienteIdx];

        if (rotaVencedora == -1) {
            copiaSolucao.rotas.push_back({clienteVencedorId});
        } else {
            copiaSolucao.rotas[rotaVencedora].insert(
                copiaSolucao.rotas[rotaVencedora].begin() + posicaoVencedora,
                clienteVencedorId
            );
        }

        naoInseridos.erase(naoInseridos.begin() + melhorClienteIdx);
    }
    return copiaSolucao;
}

static ResultadoDestruicao destruirAleatorio(const Solution& solucao, const VRPInstance& instance, double rhoMin, double rhoMax, std::mt19937& gen) {
    (void)instance; 
    Solution copiaSolucao = solucao;
    ResultadoDestruicao resultado;
    std::vector<int> listaClientes;
    std::uniform_real_distribution<> intensidadeDestruicao(rhoMin, rhoMax);
    double rho = intensidadeDestruicao(gen);

    for (size_t i = 0; i < solucao.rotas.size(); ++i) {
        for (size_t j = 0; j < solucao.rotas[i].size(); ++j) {
            listaClientes.push_back(solucao.rotas[i][j]);
        }
    }

    int nClientes = static_cast<int>(listaClientes.size());
    if (nClientes == 0) return resultado;
    int quantosRemover = std::min(nClientes, std::max(1, static_cast<int>(rho * nClientes)));

    std::shuffle(listaClientes.begin(), listaClientes.end(), gen);

    for (int i = 0; i < quantosRemover; ++i) {
        int clienteId = listaClientes[i];
        for (size_t r = 0; r < copiaSolucao.rotas.size(); ++r) {
            auto it = std::find(copiaSolucao.rotas[r].begin(), copiaSolucao.rotas[r].end(), clienteId);
            if (it != copiaSolucao.rotas[r].end()) {
                copiaSolucao.rotas[r].erase(it);
                resultado.clientes.push_back(clienteId);
                break;
            }
        }
    }

    resultado.solucao = copiaSolucao;
    return resultado;
}

static ResultadoDestruicao destruirPiorCusto(const Solution& solucao, const VRPInstance& instance, double rhoMin, double rhoMax, std::mt19937& gen) {
    Solution copiaSolucao = solucao;
    ResultadoDestruicao resultado;
    std::vector<std::pair<int, double>> custosClientes;
    const Node& depot = instance.getDepot();

    for (size_t i = 0; i < copiaSolucao.rotas.size(); ++i) {
        int tamanhoRota = static_cast<int>(copiaSolucao.rotas[i].size());
        
        for (int r = 0; r < tamanhoRota; ++r) {
            int clienteId = copiaSolucao.rotas[i][r];
            const Node& cliente = instance.getNode(clienteId);

            const Node& antes = (r == 0) ? depot : instance.getNode(copiaSolucao.rotas[i][r - 1]);
            const Node& depois = (r == tamanhoRota - 1) ? depot : instance.getNode(copiaSolucao.rotas[i][r + 1]);

            double custoRemocao = instance.distancia(antes, cliente) + 
                                  instance.distancia(cliente, depois) - 
                                  instance.distancia(antes, depois);

            custosClientes.push_back({clienteId, custoRemocao});
        }
    }

    int nClientes = static_cast<int>(custosClientes.size());
    if (nClientes == 0) return resultado;

    std::uniform_real_distribution<> intensidadeDestruicao(rhoMin, rhoMax);
    double rho = intensidadeDestruicao(gen);
    int quantosRemover = std::min(nClientes, std::max(1, static_cast<int>(rho * nClientes)));

    std::sort(custosClientes.begin(), custosClientes.end(), 
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                  return a.second > b.second; 
              });

    int tamanhoJanela = std::min(nClientes, static_cast<int>(quantosRemover * 1.5));
    std::shuffle(custosClientes.begin(), custosClientes.begin() + tamanhoJanela, gen);

    std::vector<int> clientesParaRemover;
    clientesParaRemover.reserve(quantosRemover);
    for (int i = 0; i < quantosRemover; ++i) {
        clientesParaRemover.push_back(custosClientes[i].first);
    }

    for (int clienteId : clientesParaRemover) {
        for (size_t r = 0; r < copiaSolucao.rotas.size(); ++r) {
            auto it = std::find(copiaSolucao.rotas[r].begin(), copiaSolucao.rotas[r].end(), clienteId);
            if (it != copiaSolucao.rotas[r].end()) {
                copiaSolucao.rotas[r].erase(it);
                resultado.clientes.push_back(clienteId);
                break;
            }
        }
    }

    resultado.solucao = copiaSolucao;
    return resultado;
}

Solution LNS(const VRPInstance& instance, double tempoLimite, int iterSemMelhoraMax, double rhoMin, double rhoMax, double tempInicial, double alpha) {
    std::mt19937& gen = rngGlobal();
    std::uniform_real_distribution<> dist(0.0, 1.0);

    std::vector<FuncDestruicao> operadoresDestruicao = { destruirAleatorio, destruirPiorCusto };
    std::vector<FuncConstrucao> operadoresConstrucao = { construirGuloso, construirRegret2 };

    std::vector<double> pesosDestruicao(operadoresDestruicao.size(), 1.0);
    std::vector<double> pesosConstrucao(operadoresConstrucao.size(), 1.0);

    std::vector<double> scoresDestruicao(operadoresDestruicao.size(), 0.0);
    std::vector<double> scoresConstrucao(operadoresConstrucao.size(), 0.0);
    
    std::vector<int> usosDestruicao(operadoresDestruicao.size(), 0);
    std::vector<int> usosConstrucao(operadoresConstrucao.size(), 0);

    auto inicio = std::chrono::steady_clock::now();
    auto tempoBest = inicio;

    Solution s = clarkeWright(instance);
    s.calculaCusto(instance);
    Solution best = s;
    expRegistraMelhora(best.custoTotal);

    double temperatura = tempInicial;
    int iterSemMelhora = 0;
    int iteracao = 0;
    bool destruicaoForte = false;
    const int limiteEstagnacao = std::max(1, iterSemMelhoraMax);

    std::cout << "ALNS inicio | Custo: " << best.custoTotal << std::endl;

    if (best.custoTotal == instance.optimal_value) {
        std::cout << "ALNS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
        return best;
    }

    while(true) {
        auto agora = std::chrono::steady_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(agora - inicio).count();
        if (tempoDecorrido >= tempoLimite) break;

        double fatorEstagnacao = std::min(1.0, static_cast<double>(iterSemMelhora) / limiteEstagnacao);
        double rhoMinAtual = rhoMin + (rhoMax - rhoMin) * fatorEstagnacao;
        double rhoMaxAlvo = std::max(rhoMax, 0.80);
        double rhoMaxAtual = rhoMax + (rhoMaxAlvo - rhoMax) * fatorEstagnacao;

        if (destruicaoForte) {
            rhoMinAtual = std::max(rhoMax, rhoMinAtual);
            rhoMaxAtual = std::max(rhoMinAtual, std::min(0.90, rhoMax + 0.25));
            destruicaoForte = false;
        }

        std::discrete_distribution<int> roletaDestruicao(pesosDestruicao.begin(), pesosDestruicao.end());
        std::discrete_distribution<int> roletaConstrucao(pesosConstrucao.begin(), pesosConstrucao.end());

        int idxDestruicao = roletaDestruicao(gen);
        int idxConstrucao = roletaConstrucao(gen);
        
        usosDestruicao[idxDestruicao]++;
        usosConstrucao[idxConstrucao]++;

        ResultadoDestruicao res = operadoresDestruicao[idxDestruicao](s, instance, rhoMinAtual, rhoMaxAtual, gen);
        Solution candidato = operadoresConstrucao[idxConstrucao](instance, res.solucao, res.clientes);
        candidato.calculaCusto(instance);

        double delta = candidato.custoTotal - s.custoTotal;
        double pontuacaoRodada = 0.0;

        if (candidato.custoTotal < best.custoTotal) {
            s = candidato;
            best = candidato;
            expRegistraMelhora(best.custoTotal);
            iterSemMelhora = 0;
            pontuacaoRodada = 25.0; //testar mudar
            tempoBest = std::chrono::steady_clock::now();
            
            std::cout << "ALNS iter " << iteracao << " | Melhor: " << best.custoTotal << " em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
            
            if (instance.optimal_value > 0 && best.custoTotal == instance.optimal_value) {
                std::cout << "ALNS: Otimo encontrado em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s" << std::endl;
                return best;
            }

        } else if (delta < 0) {
            s = candidato;
            iterSemMelhora = 0;
            pontuacaoRodada = 15.0; //testar mudar
        } else if (temperatura > 0 && dist(gen) < std::exp(-delta / temperatura)) {
            s = candidato;
            iterSemMelhora++;
            pontuacaoRodada = 10.0; //testar mudar
        } else {
            iterSemMelhora++;
            pontuacaoRodada = 0.0;
        }

        scoresDestruicao[idxDestruicao] += pontuacaoRodada;
        scoresConstrucao[idxConstrucao] += pontuacaoRodada;

        if (iteracao > 0 && iteracao % 150 == 0) { //testar mudar
            double r = 0.15; // reacao testar mudar

            for (size_t i = 0; i < pesosDestruicao.size(); i++) {
                if (usosDestruicao[i] > 0) {
                    pesosDestruicao[i] = (1.0 - r) * pesosDestruicao[i] + r * (scoresDestruicao[i]);
                }
                scoresDestruicao[i] = 0.0;
                usosDestruicao[i] = 0;
            }

            for (size_t i = 0; i < pesosConstrucao.size(); ++i) {
                if (usosConstrucao[i] > 0) {
                    pesosConstrucao[i] = (1.0 - r) * pesosConstrucao[i] + r * (scoresConstrucao[i] / usosConstrucao[i]);
                }
                scoresConstrucao[i] = 0.0;
                usosConstrucao[i] = 0;
            }
        }

        temperatura *= alpha;
        if (iterSemMelhora >= limiteEstagnacao) {
            s = best;
            temperatura = tempInicial;
            iterSemMelhora = 0;
            destruicaoForte = true;
        }

        iteracao++;
    }

    auto fim = std::chrono::steady_clock::now();
    std::cout << "ALNS: Melhor=" << best.custoTotal << " em " << std::chrono::duration<double>(tempoBest - inicio).count() << "s (total: " << std::chrono::duration<double>(fim - inicio).count() << "s)" << std::endl;

    return best;
}
