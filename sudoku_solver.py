"""Solucionador sequencial de Sudoku: backtracking com MRV.

Interface principal:
    resolver(tabuleiro) -> ResultadoSolver
O tabuleiro de entrada NUNCA é modificado.
"""
import argparse
import statistics
import sys
from copy import deepcopy
from dataclasses import dataclass
from time import perf_counter
from typing import List, Optional, Tuple

from sudoku_io import SudokuFormatError, carregar, encontrar_conflitos
from validar import solucao_valida

Matriz = List[List[int]]


@dataclass
class ResultadoSolver:
    resolvido: bool
    solucao: Optional[Matriz]   # None se não resolvido
    nos: int                    # valores tentados (mede esforço de busca)
    motivo: str = ""            # preenchido quando não resolvido


def candidatos(tab: Matriz, i: int, j: int) -> List[int]:
    """Valores 1..9 que ainda podem ir na célula (i, j)."""
    usados = set(tab[i])
    usados.update(tab[r][j] for r in range(9))
    bi, bj = 3 * (i // 3), 3 * (j // 3)
    usados.update(tab[r][c]
                  for r in range(bi, bi + 3) for c in range(bj, bj + 3))
    return [v for v in range(1, 10) if v not in usados]


def escolher_celula_mrv(tab: Matriz) -> Optional[Tuple[int, int, List[int]]]:
    """Célula vazia com menos candidatos (MRV).

    Retorna None se não há células vazias. Se achar uma célula com 0
    candidatos, retorna já (beco sem saída: não adianta procurar mais).
    """
    melhor = None
    for i in range(9):
        for j in range(9):
            if tab[i][j] != 0:
                continue
            cands = candidatos(tab, i, j)
            if melhor is None or len(cands) < len(melhor[2]):
                melhor = (i, j, cands)
                if not cands:
                    return melhor
    return melhor


def resolver(tabuleiro: Matriz) -> ResultadoSolver:
    """Resolve um Sudoku. Não altera `tabuleiro`."""
    conflitos = encontrar_conflitos(tabuleiro)
    if conflitos:
        return ResultadoSolver(False, None, 0,
                               "conflito nas pistas: " + "; ".join(conflitos))

    tab = deepcopy(tabuleiro)   # estado próprio: a entrada fica intacta
    nos = 0

    def busca() -> bool:
        nonlocal nos
        escolha = escolher_celula_mrv(tab)
        if escolha is None:         # nenhuma célula vazia: resolvido
            return True
        i, j, cands = escolha
        for v in cands:
            nos += 1
            tab[i][j] = v
            if busca():
                return True
            tab[i][j] = 0           # retrocesso
        return False

    if busca():
        return ResultadoSolver(True, tab, nos)
    return ResultadoSolver(False, None, nos, "sem solução")


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Resolve Sudokus sequencialmente (backtracking + MRV).")
    ap.add_argument("caminho", nargs="?", default="dados/sudokus.txt",
                    help="arquivo .txt ou .csv (padrão: dados/sudokus.txt)")
    ap.add_argument("-n", "--quantidade", type=int, default=None,
                    help="quantos Sudokus ler do CSV (se omitido, pergunta)")
    ap.add_argument("--inicio", type=int, default=0,
                    help="pular as N primeiras linhas de dados do CSV")
    ap.add_argument("--mostrar", action="store_true",
                    help="imprimir a matriz resolvida de cada Sudoku")
    args = ap.parse_args()

    try:
        sudokus = carregar(args.caminho, args.quantidade, args.inicio)
    except (OSError, SudokuFormatError) as e:
        print(f"ERRO: {e}")
        return 1

    print(f"{len(sudokus)} Sudoku(s) carregado(s) de {args.caminho}\n")

    tempos: List[float] = []
    resolvidos = validos = batem = com_gabarito = 0
    t_total = perf_counter()
    for s in sudokus:
        t0 = perf_counter()
        r = resolver(s.tabuleiro)
        dt = perf_counter() - t0
        tempos.append(dt)

        if r.resolvido:
            resolvidos += 1
            ok, erros = solucao_valida(r.solucao, s.tabuleiro)
            validos += ok
            status = "RESOLVIDO e VÁLIDO" if ok else f"INVÁLIDO: {erros}"
            if s.gabarito is not None:
                com_gabarito += 1
                if r.solucao == s.gabarito:
                    batem += 1
                else:
                    status += " (DIFERE do gabarito do CSV)"
        else:
            status = f"NÃO RESOLVIDO ({r.motivo})"
        print(f"[{s.id}] {s.nome}: {status} | nós={r.nos} | {dt:.4f}s")
        if args.mostrar and r.resolvido:
            for linha in r.solucao:
                print("  " + " ".join(map(str, linha)))
            print()
    t_total = perf_counter() - t_total

    print("\n--- Resumo ---")
    print(f"Sudokus: {len(sudokus)} | resolvidos: {resolvidos} | "
          f"válidos: {validos}")
    if com_gabarito:
        print(f"Iguais ao gabarito do CSV: {batem}/{com_gabarito}")
    print(f"Tempo total: {t_total:.4f}s | "
          f"mediana: {statistics.median(tempos):.4f}s | "
          f"máximo: {max(tempos):.4f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
