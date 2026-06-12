#include "utils/experimento.h"
#include "utils/rng.h"
#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include <unordered_set>
#include <chrono>
#include <numeric>
#include <set>
#include <utility>
#include "core/Solution.h"
#include "metaheuristicas/ga.h"
#include "neighborhoods/2opt.h"
#include "neighborhoods/2optStar.h"
#include "neighborhoods/orOpt.h"
#include "neighborhoods/relocate.h"
#include "neighborhoods/swap.h"
#include "neighborhoods/crossE.h"

static Solution gerarSolucaoAleatoria(const VRPInstance& instance, std::mt19937& gen) {
	std::vector<int> ids;
	std::vector<int> rotaAtual;
	int demandaAcumulada = 0;
	Solution solucao;

	for (size_t i = 1; i < instance.nodes.size(); i++) {
		ids.push_back(instance.nodes[i].id);
	}

	std::shuffle(ids.begin(), ids.end(), gen);

	for (size_t i = 0; i < ids.size(); i++) {
		if (demandaAcumulada + instance.getNode(ids[i]).demanda <= instance.capacity) {
			rotaAtual.push_back(ids[i]);
			demandaAcumulada += instance.getNode(ids[i]).demanda;
		} else {
			solucao.rotas.push_back(rotaAtual);
			rotaAtual.clear();
			demandaAcumulada = 0;
			rotaAtual.push_back(ids[i]);
			demandaAcumulada += instance.getNode(ids[i]).demanda;
		}
	}

	solucao.rotas.push_back(rotaAtual);

	solucao.rotas.erase(
	    std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
	        [](const std::vector<int>& rota) { return rota.empty(); }),
	    solucao.rotas.end());

	return solucao;
}

static int selecaoTorneioIndice(const std::vector<Solution>& solucoes, int tamanhoTorneio, std::mt19937& gen) {
    std::uniform_int_distribution<> torneio(0, static_cast<int>(solucoes.size()) - 1);

    int melhorIndice = torneio(gen);

    for (int i = 1; i < tamanhoTorneio; i++) {
        int indice = torneio(gen);
        if (solucoes[indice].custoTotal < solucoes[melhorIndice].custoTotal) {
            melhorIndice = indice;
        }
    }

    return melhorIndice;
}

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

static int cargaRota(const std::vector<int>& rota, const VRPInstance& instance) {
    int carga = 0;
    for (int cliente : rota) carga += instance.getNode(cliente).demanda;
    return carga;
}

static Solution buscaLocalGA(Solution solucao, const VRPInstance& instance) {
    std::vector<Solution (*)(Solution, const VRPInstance&)> vizinhancas = {
        opt2,
        relocate,
        orOptIntra3,
        orOptInter3,
        swapIntra,
        swapInter,
        crossExchange,
        opt2Star
    };

    solucao.calculaCusto(instance);
    int k = 0;
    while (k < static_cast<int>(vizinhancas.size())) {
        limpaRotasVazias(solucao);
        Solution candidata = vizinhancas[k](solucao, instance);
        limpaRotasVazias(candidata);
        candidata.calculaCusto(instance);
        if (candidata.custoTotal < solucao.custoTotal) {
            solucao = std::move(candidata);
            k = 0;
        } else {
            k++;
        }
    }

    return solucao;
}

static void insereMelhorPosicao(Solution& solucao, int cliente, const VRPInstance& instance) {
    const Node& depot = instance.getDepot();
    const Node& node = instance.getNode(cliente);
    double melhorDelta = std::numeric_limits<double>::infinity();
    int melhorRota = -1;
    int melhorPos = -1;

    std::vector<int> cargas(solucao.rotas.size(), 0);
    for (size_t r = 0; r < solucao.rotas.size(); r++) cargas[r] = cargaRota(solucao.rotas[r], instance);

    for (size_t r = 0; r < solucao.rotas.size(); r++) {
        if (cargas[r] + node.demanda > instance.capacity) continue;
        for (int pos = 0; pos <= static_cast<int>(solucao.rotas[r].size()); pos++) {
            const Node& antes = (pos == 0) ? depot : instance.getNode(solucao.rotas[r][pos - 1]);
            const Node& depois = (pos == static_cast<int>(solucao.rotas[r].size())) ? depot : instance.getNode(solucao.rotas[r][pos]);
            double delta = instance.distancia(antes, node) + instance.distancia(node, depois) - instance.distancia(antes, depois);
            if (delta < melhorDelta) {
                melhorDelta = delta;
                melhorRota = static_cast<int>(r);
                melhorPos = pos;
            }
        }
    }

    if (melhorRota == -1) {
        solucao.rotas.push_back({cliente});
    } else {
        solucao.rotas[melhorRota].insert(solucao.rotas[melhorRota].begin() + melhorPos, cliente);
    }
}

static Solution crossoverPreservaRotas(const Solution& pai1, const Solution& pai2, const VRPInstance& instance, std::mt19937& gen) {
    Solution filho;
    std::unordered_set<int> usados;
    std::vector<std::vector<int>> rotasPai1 = pai1.rotas;
    std::shuffle(rotasPai1.begin(), rotasPai1.end(), gen);

    std::uniform_real_distribution<double> prob(0.0, 1.0);
    for (const auto& rota : rotasPai1) {
        if (rota.empty() || prob(gen) > 0.55) continue;
        int carga = cargaRota(rota, instance);
        if (carga > instance.capacity) continue;
        bool conflita = false;
        for (int cliente : rota) {
            if (usados.count(cliente) > 0) {
                conflita = true;
                break;
            }
        }
        if (conflita) continue;
        filho.rotas.push_back(rota);
        usados.insert(rota.begin(), rota.end());
    }

    std::vector<int> ordemRestante;
    for (const auto& rota : pai2.rotas) {
        for (int cliente : rota) {
            if (usados.count(cliente) == 0) {
                ordemRestante.push_back(cliente);
                usados.insert(cliente);
            }
        }
    }

    for (const auto& node : instance.nodes) {
        if (node.id != instance.depot_id && usados.count(node.id) == 0) {
            ordemRestante.push_back(node.id);
        }
    }

    for (int cliente : ordemRestante) {
        insereMelhorPosicao(filho, cliente, instance);
    }

    limpaRotasVazias(filho);
    return filho;
}

static std::string assinaturaSolucao(const Solution& solucao) {
    std::vector<std::vector<int>> rotas = solucao.rotas;
    for (auto& rota : rotas) {
        if (!rota.empty() && rota.front() > rota.back()) {
            std::reverse(rota.begin(), rota.end());
        }
    }
    std::sort(rotas.begin(), rotas.end());

    std::string chave;
    for (const auto& rota : rotas) {
        for (int cliente : rota) {
            chave += std::to_string(cliente);
            chave += ',';
        }
        chave += '|';
    }
    return chave;
}

static bool muitoParecida(const Solution& a, const Solution& b) {
    if (a.rotas.size() != b.rotas.size()) return false;
    if (std::abs(a.custoTotal - b.custoTotal) > 2) return false;
    return assinaturaSolucao(a) == assinaturaSolucao(b);
}

Solution GeneticAlgorithm(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int tamanhoTorneio, int elitismo, double probMutacao, double probCrossover, double tempoLimite) {
	std::vector<Solution> solucoes;
	    std::mt19937& gen = rngGlobal();
    std::uniform_real_distribution<> probDist(0.0, 1.0);

	for (int i = 0; i < tamanhoPopulacao; i++) {
		solucoes.push_back(gerarSolucaoAleatoria(instance, gen));
	}

	for (size_t i = 0; i < solucoes.size(); i++) {
		solucoes[i].calculaCusto(instance);
        if (i < static_cast<size_t>(std::max(1, tamanhoPopulacao / 10))) {
            solucoes[i] = buscaLocalGA(std::move(solucoes[i]), instance);
        }
	}

    auto melhorGlobal = std::min_element(solucoes.begin(), solucoes.end(),
        [](const Solution& a, const Solution& b) {
            return a.custoTotal < b.custoTotal;
        });

    Solution melhorSolucao = *melhorGlobal;
    expRegistraMelhora(melhorSolucao.custoTotal);

    std::cout << "Geracao 0 (inicial) | Melhor custo: " << melhorSolucao.custoTotal << std::endl;

	auto inicio = std::chrono::steady_clock::now();
	auto tempoBest = inicio;
    int geracoesSemMelhora = 0;

	for (int i = 0; i < numGeracoes; i++) {
		if (tempoLimite > 0.0 &&
		    std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count() >= tempoLimite) {
			break;
		}
		std::nth_element(solucoes.begin(), solucoes.begin() + elitismo, solucoes.end(),
    		[](const Solution& a, const Solution& b) {
        		return a.custoTotal < b.custoTotal;
    		});

		std::vector<Solution> novaGeracao(solucoes.begin(), solucoes.begin() + elitismo);
        for (Solution& eliteSol : novaGeracao) {
            eliteSol = buscaLocalGA(std::move(eliteSol), instance);
        }
        std::set<std::string> assinaturas;
        for (const Solution& eliteSol : novaGeracao) {
            assinaturas.insert(assinaturaSolucao(eliteSol));
        }

		while (static_cast<int>(novaGeracao.size()) < tamanhoPopulacao) {
	        int idx1 = selecaoTorneioIndice(solucoes, tamanhoTorneio, gen);
		    int idx2 = selecaoTorneioIndice(solucoes, tamanhoTorneio, gen);

		    int tentativas = 0;
		    while (idx1 == idx2 && tentativas < 10) {
		        idx2 = selecaoTorneioIndice(solucoes, tamanhoTorneio, gen);
		        tentativas++;
		    }

		    Solution filho;

		    if (probDist(gen) < probCrossover && idx1 != idx2) {
		        filho = crossoverPreservaRotas(solucoes[idx1], solucoes[idx2], instance, gen);
			    } else {
			        filho = solucoes[idx1];
			        //limpaRotasVazias(filho);
			        filho = swapIntra(std::move(filho), instance);
			        filho = swapInter(std::move(filho), instance);
			        // filho = opt2(filho, instance);
			        filho = crossExchange(std::move(filho), instance);
			    }

                double mutacaoAtual = probMutacao;
                if (geracoesSemMelhora > 80) mutacaoAtual = std::min(0.85, probMutacao * 2.0);
                if (geracoesSemMelhora > 160) mutacaoAtual = std::min(0.95, probMutacao * 3.0);

			    if (probDist(gen) < mutacaoAtual) {
		            filho = randomRelocate(std::move(filho), instance);
		            filho = randomOpt2(std::move(filho), instance);
		            filho = randomSwapIntra(std::move(filho), instance);
		            filho = randomSwapInter(std::move(filho), instance);
                    filho = randomOrOptInter(std::move(filho), instance, 3);
                    filho = randomOpt2Star(std::move(filho), instance);
	    	}

	    	if (probDist(gen) < 0.45 || geracoesSemMelhora > 60) {

	    		limpaRotasVazias(filho);
                filho = buscaLocalGA(std::move(filho), instance);
	    	}

			limpaRotasVazias(filho);
	    	filho.calculaCusto(instance);

            std::string assinatura = assinaturaSolucao(filho);
            bool duplicada = assinaturas.count(assinatura) > 0;
            if (!duplicada) {
                for (const Solution& existente : novaGeracao) {
                    if (muitoParecida(filho, existente)) {
                        duplicada = true;
                        break;
                    }
                }
            }

            if (duplicada) {
                filho = gerarSolucaoAleatoria(instance, gen);
                filho = randomRelocate(std::move(filho), instance);
                filho = randomOpt2(std::move(filho), instance);
                filho = buscaLocalGA(std::move(filho), instance);
                assinatura = assinaturaSolucao(filho);
            }

            assinaturas.insert(assinatura);
	    	novaGeracao.push_back(filho);
    	}

    	solucoes = novaGeracao;

        auto melhorAtual = std::min_element(solucoes.begin(), solucoes.end(),
            [](const Solution& a, const Solution& b) {
                return a.custoTotal < b.custoTotal;
            });

        if (melhorAtual->custoTotal < melhorSolucao.custoTotal) {
            melhorSolucao = *melhorAtual;
            expRegistraMelhora(melhorSolucao.custoTotal);
            tempoBest = std::chrono::steady_clock::now();
            geracoesSemMelhora = 0;
        } else {
            geracoesSemMelhora++;
        }

        if (geracoesSemMelhora > 0 && geracoesSemMelhora % 120 == 0) {
            std::sort(solucoes.begin(), solucoes.end(),
                [](const Solution& a, const Solution& b) { return a.custoTotal < b.custoTotal; });
            int reiniciar = std::max(1, tamanhoPopulacao * 30 / 100);
            for (int p = tamanhoPopulacao - reiniciar; p < tamanhoPopulacao; p++) {
                solucoes[static_cast<size_t>(p)] = gerarSolucaoAleatoria(instance, gen);
                solucoes[static_cast<size_t>(p)] = buscaLocalGA(std::move(solucoes[static_cast<size_t>(p)]), instance);
            }
        }

        if (instance.optimal_value > 0 && melhorSolucao.custoTotal == instance.optimal_value) break;

        if ((i + 1) % 100 == 0 || i == numGeracoes - 1) {
            std::cout << "Geracao " << i + 1 
                      << " | Melhor geracao: " << melhorAtual->custoTotal 
                      << " | Melhor global: " << melhorSolucao.custoTotal << std::endl;
        }
	}

	auto fim = std::chrono::steady_clock::now();
	double tempoTotal = std::chrono::duration<double>(fim - inicio).count();
	double tempoMelhor = std::chrono::duration<double>(tempoBest - inicio).count();

	if (instance.optimal_value > 0 && melhorSolucao.custoTotal == instance.optimal_value)
	    std::cout << "GA: Otimo encontrado em " << tempoMelhor << "s" << std::endl;
	else
	    std::cout << "GA: Melhor=" << melhorSolucao.custoTotal << " em " << tempoMelhor << "s (total: " << tempoTotal << "s)" << std::endl;

	return melhorSolucao;
}
