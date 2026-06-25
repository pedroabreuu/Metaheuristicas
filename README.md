# Metaheurísticas para CVRP

Implementações de metaheurísticas para o CVRP, usando instâncias no formato da CVRPLib.

O projeto principal está na pasta `cvrp/`.

## Dependências

Para compilar e executar o projeto, você precisa de:

- compilador C++ com suporte a C++17 ou superior;
- CMake;
- GNU parallel, recomendado para experimentos em lote;
- tmux ou nohup, opcional.

## Metaheurísticas disponíveis

O executável aceita os seguintes algoritmos:

```text
sa
ga
brkga
vns
grasp
ils
lns
pso
```

## Como compilar

A partir da raiz deste repositório:

```bash
cd cvrp
```

Configure o build em modo Release:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Compilar:

```bash
cmake --build build -j
```

O binário será gerado em:

```text
cvrp/build/cvrp
```

Depois da primeira configuração com CMake, normalmente basta recompilar com:

```bash
cmake --build build -j
```

## Como rodar uma execução individual

Formato geral:

```bash
./build/cvrp <instancia.vrp> <algoritmo> [seed] [--tempo segundos] [--csv arquivo]
```

Exemplo com uma instância da classe A, algoritmo VNS, seed fixa e limite de 400 segundos:

```bash
./build/cvrp data/A/A-n80-k10.vrp vns 873654221 --tempo 400
```

Exemplo salvando o resultado final em CSV:

```bash
./build/cvrp data/A/A-n80-k10.vrp vns 873654221 \
  --tempo 400 \
  --csv resultados/teste/raw.csv
```

O arquivo passado em `--csv` recebe uma linha com:

```text
instancia,algo,seed,custo,gap,tempo_melhor,tempo_total
```


## Como rodar experimentos em lote

O script principal para rodar é:

```text
cvrp/scripts/experimentos.sh
```

Ele executa combinações de:

- instâncias;
- algoritmos;
- seeds;
- tempo limite;
- número de jobs paralelos.

O script gera:

```text
resultados/<experimento>/raw/          # uma linha por execução
resultados/<experimento>/logs/         # saída completa de cada execução
resultados/<experimento>/joblog.tsv    # log do GNU parallel
resultados/<experimento>/resultados.csv
```

### Exemplo: uma instância, todos os algoritmos, 100 seeds

```bash
cd cvrp
```

```bash
OUT="resultados/A-n80-k10_100seeds_400s"
mkdir -p "$OUT"
export OUT
```

```bash
export INSTANCIAS="data/A/A-n80-k10.vrp"
```

```bash
export ALGOS="sa ga brkga vns grasp ils lns pso"
```

```bash
export SEEDS="$(awk '!seen[$1]++ {print $1; n++; if (n==100) exit}' ../Sementes_Taillard.txt | tr -d '\r' | paste -sd ' ' -)"
```

```bash
export TEMPO=400
```

```bash
export JOBS=6
```

```bash
bash scripts/experimentos.sh > "$OUT/runner.log" 2>&1
```


