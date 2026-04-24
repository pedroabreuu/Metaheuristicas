#!/bin/bash

set -euo pipefail

DATA_DIR="data"
BIN="./build/cvrp"
TIMEOUT=0
OUTPUT_DIR="results"
GROUP=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --group)     GROUP="$2";     shift 2 ;;
        --data-dir)  DATA_DIR="$2";  shift 2 ;;
        --bin)       BIN="$2";       shift 2 ;;
        --timeout)   TIMEOUT="$2";   shift 2 ;;
        --help|-h)
            echo "Uso: $0 --group <LETRA> [--data-dir DIR] [--bin BIN] [--timeout SEG]"
            echo "Exemplo: $0 --group A"
            echo "Exemplo: $0 --group B --timeout 120"
            exit 0 ;;
        *) echo "Opção desconhecida: $1"; exit 1 ;;
    esac
done

if [[ -z "$GROUP" ]]; then
    echo "[ERRO] Informe o grupo com --group <LETRA>  (ex: --group A)"
    exit 1
fi

if [[ -d "${DATA_DIR}/${GROUP}" ]]; then
    SEARCH_DIR="${DATA_DIR}/${GROUP}"
else
    SEARCH_DIR="${DATA_DIR}"
fi

if [[ ! -x "$BIN" ]]; then
    echo "[ERRO] Binário não encontrado ou sem permissão: $BIN"
    echo "       Compile primeiro: cmake --build build --config Release"
    exit 1
fi

if [[ ! -d "$SEARCH_DIR" ]]; then
    echo "[ERRO] Diretório de dados não encontrado: $SEARCH_DIR"
    exit 1
fi

GROUP_OUTPUT="${OUTPUT_DIR}/${GROUP}"
mkdir -p "$GROUP_OUTPUT"
LOG_FILE="${GROUP_OUTPUT}/summary.csv"

echo "instancia,otimo_conhecido,custo_encontrado,gap_%,tempo_s,status" > "$LOG_FILE"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'

total=0; ok=0; failed=0; timed_out=0

echo "============================================================"
echo " Grupo    : $GROUP"
echo " Dados    : $SEARCH_DIR"
echo " Binário  : $BIN"
echo " Timeout  : ${TIMEOUT}s (0 = sem limite)"
echo " Resultados: $GROUP_OUTPUT/"
echo "============================================================"

for vrp_file in "$SEARCH_DIR"/${GROUP}-*.vrp; do
    [[ -f "$vrp_file" ]] || { echo "[AVISO] Nenhuma instância ${GROUP}-*.vrp encontrada em $SEARCH_DIR"; break; }

    instance=$(basename "$vrp_file" .vrp)
    out_file="${GROUP_OUTPUT}/${instance}.txt"
    total=$((total + 1))

    printf "%-20s ... " "$instance"

    t_start=$(date +%s%N)

    if [[ "$TIMEOUT" -gt 0 ]]; then
        exit_code=0
        timeout "$TIMEOUT" "$BIN" "$vrp_file" > "$out_file" 2>&1 || exit_code=$?
    else
        exit_code=0
        "$BIN" "$vrp_file" > "$out_file" 2>&1 || exit_code=$?
    fi

    t_end=$(date +%s%N)
    elapsed=$(echo "scale=7; ($t_end - $t_start) / 1000000000" | bc | sed 's/^\./0./')

    if [[ "$exit_code" == "124" ]]; then
        echo -e "${YELLOW}TIMEOUT${NC} (${elapsed}s)"
        echo "${instance},,,, ${elapsed},timeout" >> "$LOG_FILE"
        timed_out=$((timed_out + 1))
        continue
    fi

    if [[ "$exit_code" != "0" ]]; then
        echo -e "${RED}ERRO (exit $exit_code)${NC}"
        echo "${instance},,,,${elapsed},erro_${exit_code}" >> "$LOG_FILE"
        failed=$((failed + 1))
        continue
    fi

    otimo=$(grep -oP '(?<=Otimo conhecido: )\d+' "$out_file" || echo "N/A")
    custo=$(grep -oP '(?<=Cost )\d+' "$out_file" | tail -1 || echo "N/A")

    if [[ "$custo" != "N/A" && "$otimo" != "N/A" && "$otimo" != "0" ]]; then
        gap=$(echo "scale=2; ($custo - $otimo) * 100 / $otimo" | bc)
    else
        gap="N/A"
    fi

    echo -e "${GREEN}OK${NC}  custo=${custo}  otimo=${otimo}  gap=${gap}%  tempo=${elapsed}s"
    echo "${instance},${otimo},${custo},${gap},${elapsed}s,ok" >> "$LOG_FILE"
    ok=$((ok + 1))
done

echo "============================================================"
echo " Resumo do grupo $GROUP"
echo "   Total      : $total"
echo -e "   Sucesso    : ${GREEN}$ok${NC}"
echo -e "   Timeout    : ${YELLOW}$timed_out${NC}"
echo -e "   Erro       : ${RED}$failed${NC}"
echo " CSV de resumo: $LOG_FILE"
echo "============================================================"