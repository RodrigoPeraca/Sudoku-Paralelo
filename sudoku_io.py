"""Leitura e validação de arquivos com vários Sudokus.

Formato:
  - 9 linhas de 9 dígitos por tabuleiro; 0 = célula vazia
  - tabuleiros separados por linhas em branco
  - linhas iniciadas por '#' são comentários (o comentário imediatamente
    antes de um tabuleiro vira o nome dele)
"""
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple

TAMANHO = 9
DIGITOS = "0123456789"


class SudokuFormatError(ValueError):
    """Entrada malformada ou com pistas conflitantes."""


@dataclass
class Sudoku:
    id: int                      # posição no arquivo, começando em 1
    nome: str
    tabuleiro: List[List[int]]   # matriz 9x9, 0 = vazio


def _unidades():
    """Gera (nome, lista de células) para cada linha, coluna e bloco 3x3."""
    for i in range(TAMANHO):
        yield f"linha {i + 1}", [(i, j) for j in range(TAMANHO)]
    for j in range(TAMANHO):
        yield f"coluna {j + 1}", [(i, j) for i in range(TAMANHO)]
    for bi in range(3):
        for bj in range(3):
            celulas = [(bi * 3 + di, bj * 3 + dj)
                       for di in range(3) for dj in range(3)]
            yield f"bloco ({bi + 1},{bj + 1})", celulas


def encontrar_conflitos(tabuleiro: List[List[int]]) -> List[str]:
    """Lista os conflitos entre pistas (valores repetidos numa linha,
    coluna ou bloco). Lista vazia = pistas consistentes."""
    conflitos = []
    for nome, celulas in _unidades():
        vistos = set()
        for i, j in celulas:
            v = tabuleiro[i][j]
            if v == 0:
                continue
            if v in vistos:
                conflitos.append(f"{nome}: valor {v} repetido")
            vistos.add(v)
    return conflitos


def _separar_blocos(texto: str) -> List[Tuple[Optional[str], int, List[str]]]:
    """Divide o texto em blocos: (nome, linha_inicial, [linhas])."""
    blocos = []
    atual: List[str] = []
    nome = None
    inicio = 0
    for n, bruta in enumerate(texto.splitlines(), start=1):
        linha = bruta.strip()
        if linha.startswith("#"):
            if not atual:  # comentário antes do bloco = nome do bloco
                nome = linha.lstrip("#").strip() or None
            continue
        if linha == "":
            if atual:
                blocos.append((nome, inicio, atual))
                atual, nome = [], None
            continue
        if not atual:
            inicio = n
        atual.append(linha)
    if atual:
        blocos.append((nome, inicio, atual))
    return blocos


def _converter_bloco(linhas: List[str], inicio: int) -> List[List[int]]:
    if len(linhas) != TAMANHO:
        raise SudokuFormatError(
            f"bloco na linha {inicio}: esperadas {TAMANHO} linhas, "
            f"encontradas {len(linhas)}")
    tabuleiro = []
    for k, linha in enumerate(linhas):
        n = inicio + k
        if len(linha) != TAMANHO:
            raise SudokuFormatError(
                f"linha {n}: esperados {TAMANHO} dígitos, "
                f"encontrados {len(linha)}")
        if any(c not in DIGITOS for c in linha):
            raise SudokuFormatError(
                f"linha {n}: só são permitidos dígitos de 0 a 9")
        tabuleiro.append([int(c) for c in linha])
    return tabuleiro


def parse_sudokus(texto: str) -> List[Sudoku]:
    blocos = _separar_blocos(texto)
    if not blocos:
        raise SudokuFormatError("nenhum Sudoku encontrado na entrada")
    sudokus = []
    for idx, (nome, inicio, linhas) in enumerate(blocos, start=1):
        tabuleiro = _converter_bloco(linhas, inicio)
        conflitos = encontrar_conflitos(tabuleiro)
        if conflitos:
            raise SudokuFormatError(
                f"Sudoku {idx}: conflito nas pistas iniciais: "
                + "; ".join(conflitos))
        sudokus.append(Sudoku(idx, nome or f"sudoku_{idx}", tabuleiro))
    return sudokus


def ler_sudokus(caminho: str) -> List[Sudoku]:
    with open(caminho, encoding="utf-8") as f:
        return parse_sudokus(f.read())


if __name__ == "__main__":
    caminho = sys.argv[1] if len(sys.argv) > 1 else "dados/sudokus.txt"
    try:
        sudokus = ler_sudokus(caminho)
    except (OSError, SudokuFormatError) as e:
        print(f"ERRO: {e}")
        sys.exit(1)
    print(f"{len(sudokus)} Sudoku(s) lido(s) de {caminho}\n")
    for s in sudokus:
        pistas = sum(1 for linha in s.tabuleiro for v in linha if v)
        print(f"[{s.id}] {s.nome} — {pistas} pistas")
        for linha in s.tabuleiro:
            print("  " + " ".join(str(v) if v else "." for v in linha))
        print()
