"""Verificação independente de soluções de Sudoku.

Não importa nada do solucionador de propósito: a validação precisa ter
lógica própria para detectar bugs dele.
"""
from typing import List, Optional, Tuple

Matriz = List[List[int]]
ESPERADO = list(range(1, 10))


def solucao_valida(solucao: Matriz,
                   original: Optional[Matriz] = None) -> Tuple[bool, List[str]]:
    """Retorna (valida, lista_de_erros).

    Confere: formato 9x9, valores 1..9, linhas/colunas/blocos sem repetição
    e, se `original` for dado, preservação das pistas iniciais.
    """
    erros: List[str] = []

    if len(solucao) != 9 or any(len(l) != 9 for l in solucao):
        return False, ["formato diferente de 9x9"]
    if any(not isinstance(v, int) or v < 1 or v > 9
           for l in solucao for v in l):
        return False, ["há valores fora do intervalo 1..9"]

    for i in range(9):
        if sorted(solucao[i]) != ESPERADO:
            erros.append(f"linha {i + 1} inválida")
    for j in range(9):
        if sorted(solucao[i][j] for i in range(9)) != ESPERADO:
            erros.append(f"coluna {j + 1} inválida")
    for bi in range(0, 9, 3):
        for bj in range(0, 9, 3):
            bloco = [solucao[i][j]
                     for i in range(bi, bi + 3) for j in range(bj, bj + 3)]
            if sorted(bloco) != ESPERADO:
                erros.append(f"bloco ({bi // 3 + 1},{bj // 3 + 1}) inválido")

    if original is not None:
        for i in range(9):
            for j in range(9):
                if original[i][j] != 0 and original[i][j] != solucao[i][j]:
                    erros.append(
                        f"pista alterada em ({i + 1},{j + 1}): "
                        f"{original[i][j]} -> {solucao[i][j]}")

    return len(erros) == 0, erros
