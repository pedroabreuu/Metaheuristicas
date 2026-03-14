# Metaheurísticas
 
Implementações de metaheurísticas aplicadas a problemas de otimização combinatória.
 
## Estrutura do repositório
 
```
Metaheuristicas/
└── cvrp/            ← Capacitated Vehicle Routing Problem
    ├── include/     ← Headers (.h)
    ├── src/         ← Código-fonte (.cpp)
    ├── data/        ← Instâncias de teste (.vrp)
    └── results/     ← Saídas geradas (ignorado pelo git)
```
 
## Problemas implementados
 
- [CVRP](cvrp/README.md) — Capacitated Vehicle Routing Problem
 
## Como compilar
 
```bash
cd cvrp
g++ src/parser.cpp -o build/parser
./build/parser
```
