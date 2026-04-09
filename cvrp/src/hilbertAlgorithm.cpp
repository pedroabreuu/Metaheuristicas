#include "hilbertAlgorithm.h"
#include <vector>
#include <algorithm>
#include <numeric>

Solution constroiSolucaoHilbert(const VRPInstance& instance, int p) {
	Solution solucao;
	std::vector<Node> nodesHilbert;

	for (size_t i = 0; i < instance.nodes.size(); i++) {
		if (instance.nodes[i].id != instance.depot_id) {
			nodesHilbert.push_back(instance.nodes[i]);
		}
	}

	std::vector<int> vetorIndicesHilbert = hilberTransform(nodesHilbert, p);

	std::vector<int> posicoes(nodesHilbert.size());
	std::iota(posicoes.begin(), posicoes.end(), 0);

	std::sort(posicoes.begin(), posicoes.end(), [&vetorIndicesHilbert](int a, int b) {
		return vetorIndicesHilbert[a] < vetorIndicesHilbert[b];
	});

	std::vector<int> rotaAtual;
	int demandaAtual = 0;

	for (int pos : posicoes) {
		int demandaCliente = nodesHilbert[pos].demanda;

		if (demandaAtual + demandaCliente > instance.capacity && !rotaAtual.empty()) {
			solucao.rotas.push_back(rotaAtual);
			rotaAtual.clear();
			demandaAtual = 0;
		}

		rotaAtual.push_back(nodesHilbert[pos].id);
		demandaAtual += demandaCliente;
	}

	if (!rotaAtual.empty()) {
		solucao.rotas.push_back(rotaAtual);
	}

	return solucao;
}