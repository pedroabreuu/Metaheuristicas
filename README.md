# Metaheurísticas

Implementações de metaheurísticas aplicadas a problemas de otimização combinatória, utilizando instâncias da [CVRPLib](http://vrp.atd-lab.inf.puc-rio.br/).

---

## Estrutura do repositório

```
Metaheuristicas/
└── cvrp/                  ← Capacitated Vehicle Routing Problem
    ├── CMakeLists.txt     ← Configuração do build system
    ├── include/           ← Headers (.h)
    │   ├── Node.h         ← Estrutura de um nó (id, coordenadas, demanda)
    │   ├── VRPInstance.h  ← Instância do problema (nós, depósito, capacidade)
    │   └── parser.h       ← Interface do parser de arquivos .vrp
    ├── src/               ← Código-fonte (.cpp)
    │   ├── main.cpp       ← Ponto de entrada
    │   └── parser.cpp     ← Implementação do parser
    ├── data/              ← Instâncias de teste (.vrp)
    └── results/           ← Saídas geradas (ignorado pelo git)
```

---

## Problemas implementados

- [CVRP](cvrp/) — Capacitated Vehicle Routing Problem

---

## Dependências

- CMake >= 3.15
- Compilador C++17 (GCC ou Clang)

Para instalar o CMake no Ubuntu/Debian:

```bash
sudo apt install cmake
```

---

## Como compilar

A partir da raiz do projeto (`cvrp/`), configure o build uma única vez:

```bash
cmake -S . -B build
```

Em seguida, compile sempre que houver alterações no código:

```bash
cmake --build build
```

O binário será gerado em `build/cvrp`.

---

## Como executar

Usando a instância padrão (`data/A-n32-k5.vrp`):

```bash
./build/cvrp
```

Passando uma instância específica como argumento:

```bash
./build/cvrp data/A-n32-k5.vrp
```

---

## Configuração do ambiente de desenvolvimento

Para que o language server (`clangd`) funcione corretamente no seu editor, gere o `compile_commands.json` e crie um link simbólico na raiz do projeto:

```bash
cmake -S . -B build
ln -s build/compile_commands.json compile_commands.json
```

---

## Formato das instâncias

O parser suporta o formato padrão da CVRPLib (`.vrp`), lendo as seguintes seções:

- `NODE_COORD_SECTION` — coordenadas de cada nó
- `DEMAND_SECTION` — demanda de cada cliente
- `DEPOT_SECTION` — identificador do depósito
- Cabeçalho — campos `CAPACITY` e número de veículos via `COMMENT`

## Como compilar
 
A partir da raiz do projeto (`cvrp/`), configure o build uma única vez:
 
```bash
cmake -S . -B build
```
 
Em seguida, compile sempre que houver alterações no código:
 
```bash
cmake --build build
```
 
O binário será gerado em `build/cvrp`.
 
---
 
## Como executar
 
Usando a instância padrão (`data/A-n32-k5.vrp`):
 
```bash
./build/cvrp
```
