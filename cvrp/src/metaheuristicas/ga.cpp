#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include <unordered_set>
#include <chrono>
#include <utility>
#include "core/Solution.h"
#include "metaheuristicas/ga.h"
#include "neighborhoods/2opt.h"
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
    std::uniform_int_distribution<> torneio(0, solucoes.size() - 1);

    int melhorIndice = torneio(gen);

    for (int i = 1; i < tamanhoTorneio; i++) {
        int indice = torneio(gen);
        if (solucoes[indice].custoTotal < solucoes[melhorIndice].custoTotal) {
            melhorIndice = indice;
        }
    }

    return melhorIndice;
}

static Solution crossoverOX(const Solution& pai1, const Solution& pai2, const VRPInstance& instance, std::mt19937& gen) {
	std::vector<int> pai1Concat;
	std::vector<int> pai2Concat;
	std::vector<int> rotaAtual;

	for (const auto& rota : pai1.rotas) {
    	pai1Concat.insert(pai1Concat.end(), rota.begin(), rota.end());
	}

	for (const auto& rota : pai2.rotas) {
    	pai2Concat.insert(pai2Concat.end(), rota.begin(), rota.end());
	}

	std::uniform_int_distribution<> dist(0, pai1Concat.size() - 1);

	int ponto1 = dist(gen);
	int ponto2 = dist(gen);

	while (ponto1 == ponto2) { ponto2 = dist(gen); }

	int corte1 = std::min(ponto1, ponto2);
	int corte2 = std::max(ponto1, ponto2);

	std::vector<int> filho(pai1Concat.size(), -1);
	std::unordered_set<int> setPai1;

	for (int i = corte1; i <= corte2; i++) {
    	filho[i] = pai1Concat[i];
    	setPai1.insert(pai1Concat[i]);
	}

	int atualFilho = (corte2 + 1) % pai1Concat.size();

    for (size_t i = 0; i < pai1Concat.size(); i++) {
        int atualPai2 = (corte2 + 1 + i) % pai1Concat.size();
        int candidato = pai2Concat[atualPai2];

        if (setPai1.count(candidato) == 0) {
            filho[atualFilho] = candidato;
            atualFilho = (atualFilho + 1) % pai1Concat.size();
        }
    }

    Solution solucao;
    int demandaAcumulada = 0;

    for (size_t i = 0; i < filho.size(); i++) {
    	if (demandaAcumulada + instance.getNode(filho[i]).demanda <= instance.capacity) {
    		rotaAtual.push_back(filho[i]);
			demandaAcumulada += instance.getNode(filho[i]).demanda;
    	} else {
    		solucao.rotas.push_back(rotaAtual);
			rotaAtual.clear();
			demandaAcumulada = 0;
			rotaAtual.push_back(filho[i]);
			demandaAcumulada += instance.getNode(filho[i]).demanda;
    	}
    }

    solucao.rotas.push_back(rotaAtual);

    solucao.rotas.erase(
	    std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
	        [](const std::vector<int>& rota) { return rota.empty(); }),
	    solucao.rotas.end());

    return solucao;
}

static void limpaRotasVazias(Solution& solucao) {
    solucao.rotas.erase(
        std::remove_if(solucao.rotas.begin(), solucao.rotas.end(),
            [](const std::vector<int>& rota) { return rota.empty(); }),
        solucao.rotas.end());
}

Solution GeneticAlgorithm(const VRPInstance& instance, int numGeracoes, int tamanhoPopulacao, int tamanhoTorneio, int elitismo, double probMutacao, double probCrossover) {
	std::vector<Solution> solucoes;
	std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> probDist(0.0, 1.0);

	for (int i = 0; i < tamanhoPopulacao; i++) {
		solucoes.push_back(gerarSolucaoAleatoria(instance, gen));
	}

	for (size_t i = 0; i < solucoes.size(); i++) {
		solucoes[i].calculaCusto(instance);
	}

    auto melhorGlobal = std::min_element(solucoes.begin(), solucoes.end(),
        [](const Solution& a, const Solution& b) {
            return a.custoTotal < b.custoTotal;
        });

    Solution melhorSolucao = *melhorGlobal;

    std::cout << "Geracao 0 (inicial) | Melhor custo: " << melhorSolucao.custoTotal << std::endl;

    auto inicio = std::chrono::steady_clock::now();
	auto tempoBest = inicio;

	for (int i = 0; i < numGeracoes; i++) {
		std::nth_element(solucoes.begin(), solucoes.begin() + elitismo, solucoes.end(),
    		[](const Solution& a, const Solution& b) {
        		return a.custoTotal < b.custoTotal;
    		});

		std::vector<Solution> novaGeracao(solucoes.begin(), solucoes.begin() + elitismo);

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
		        filho = crossoverOX(solucoes[idx1], solucoes[idx2], instance, gen);
			    } else {
			        filho = solucoes[idx1];
			        //limpaRotasVazias(filho);
			        filho = swapIntra(std::move(filho), instance);
			        filho = swapInter(std::move(filho), instance);
			        // filho = opt2(filho, instance);
			        filho = crossExchange(std::move(filho), instance);
			    }

			    if (probDist(gen) < probMutacao) {
		            filho = randomRelocate(std::move(filho), instance);
		            filho = randomOpt2(std::move(filho), instance);
		            filho = randomSwapIntra(std::move(filho), instance);
		            filho = randomSwapInter(std::move(filho), instance);
	    	}

	    	if (probDist(gen) < 0.2) {

	    		limpaRotasVazias(filho);
				filho = crossExchange(std::move(filho), instance);

				limpaRotasVazias(filho);
		    	filho = swapIntra(std::move(filho), instance);

		    	limpaRotasVazias(filho);
		    	filho = swapInter(std::move(filho), instance);

		    	limpaRotasVazias(filho);	
		    	filho = opt2(std::move(filho), instance);

	    		limpaRotasVazias(filho);
				filho = relocate(std::move(filho), instance);

				limpaRotasVazias(filho);
		    	filho = swapIntra(std::move(filho), instance);

		    	limpaRotasVazias(filho);
		    	filho = swapInter(std::move(filho), instance);

		    	limpaRotasVazias(filho);
				filho = crossExchange(std::move(filho), instance);
	    	}

			limpaRotasVazias(filho);
	    	filho.calculaCusto(instance);
	    	novaGeracao.push_back(filho);
    	}

    	solucoes = novaGeracao;

        auto melhorAtual = std::min_element(solucoes.begin(), solucoes.end(),
            [](const Solution& a, const Solution& b) {
                return a.custoTotal < b.custoTotal;
            });

        if (melhorAtual->custoTotal < melhorSolucao.custoTotal) {
            melhorSolucao = *melhorAtual;
            tempoBest = std::chrono::steady_clock::now();
        }

        if (instance.optimal_value > 0 && melhorSolucao.custoTotal == instance.optimal_value) break;

        if ((i + 1) % 1 == 0 || i == numGeracoes - 1) {
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
