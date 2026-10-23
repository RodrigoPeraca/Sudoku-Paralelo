#!/usr/bin/env bash
# Confere que o driver OpenMP produz o MESMO resultado do sequencial
# (contagens e nós totais) para vários números de threads.
# uso: ./verificar_omp.sh [arquivo.csv] [quantidade]
set -u
CSV=${1:-../dados/sudoku.csv}
N=${2:-20000}
filtrar() { grep -E '^(Sudokus:|Iguais|Nós)'; }

ref=$(./sudoku_seq "$CSV" -n "$N" | filtrar)
echo "referência (sequencial):"; echo "$ref" | sed 's/^/  /'; echo

falhou=0
for t in 1 2 4 8; do
  for s in static dynamic guided; do
    out=$(./sudoku_omp "$CSV" -n "$N" -t "$t" --sched "$s" | filtrar)
    if [ "$out" = "$ref" ]; then
      echo "ok     threads=$t sched=$s"
    else
      echo "FALHA  threads=$t sched=$s"; echo "$out" | sed 's/^/  /'
      falhou=1
    fi
  done
done
exit $falhou