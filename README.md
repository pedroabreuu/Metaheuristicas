# Metaheurísticas

Implementações de metaheurísticas aplicadas a problemas de otimização combinatória, utilizando instâncias da [CVRPLib](http://vrp.atd-lab.inf.puc-rio.br/).

---

## Como compilar

A partir da raiz do projeto (`cvrp/`), configure o build uma única vez:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Em seguida, compile sempre que houver alterações no código:

```bash
cmake --build build -j
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


