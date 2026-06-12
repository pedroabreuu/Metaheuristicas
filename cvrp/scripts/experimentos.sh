#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "$0")/.."

INSTANCIAS=${INSTANCIAS:-"data/A/A-n32-k5.vrp data/A/A-n80-k10.vrp data/X/X-n101-k25.vrp"}
ALGOS=${ALGOS:-"sa ga brkga vns grasp ils lns pso"}
SEEDS=${SEEDS:-$(seq 1 30)}
TEMPO=${TEMPO:-300}
JOBS=${JOBS:-$(nproc)}
OUT=${OUT:-resultados}

if [ ! -x build/cvrp ]; then
    echo "erro: build/cvrp nao encontrado; compile antes (cmake --build build)" >&2
    exit 1
fi

mkdir -p "$OUT/raw" "$OUT/traj" "$OUT/logs"

echo "instancias: $INSTANCIAS"
echo "algoritmos: $ALGOS"
echo "seeds:      $(echo $SEEDS | tr '\n' ' ')"
echo "tempo:      ${TEMPO}s | jobs: $JOBS"

if command -v parallel >/dev/null; then
    parallel -j "$JOBS" --joblog "$OUT/joblog.tsv" --resume-failed --bar \
        "./build/cvrp {1} {2} {3} --tempo $TEMPO \
            --csv  $OUT/raw/{2}_{1/.}_{3}.csv \
            --traj $OUT/traj/{2}_{1/.}_{3}.csv \
            > $OUT/logs/{2}_{1/.}_{3}.log 2>&1" \
        ::: $INSTANCIAS ::: $ALGOS ::: $SEEDS
else
    echo "aviso: GNU parallel nao encontrado; usando xargs -P $JOBS" >&2
    for inst in $INSTANCIAS; do
        nome=$(basename "$inst" .vrp)
        for algo in $ALGOS; do
            for seed in $SEEDS; do
                printf '%s\t%s\t%s\t%s\n' "$inst" "$algo" "$seed" "$nome"
            done
        done
    done | xargs -P "$JOBS" -n 4 sh -c '
        ./build/cvrp "$1" "$2" "$3" --tempo '"$TEMPO"' \
            --csv  '"$OUT"'/raw/"$2"_"$4"_"$3".csv \
            --traj '"$OUT"'/traj/"$2"_"$4"_"$3".csv \
            > '"$OUT"'/logs/"$2"_"$4"_"$3".log 2>&1' sh
fi

{
    echo "instancia,algo,seed,custo,gap,tempo_melhor,tempo_total"
    cat "$OUT"/raw/*.csv
} > "$OUT/resultados.csv"

echo "ok: $(($(wc -l < "$OUT/resultados.csv") - 1)) execucoes em $OUT/resultados.csv"
