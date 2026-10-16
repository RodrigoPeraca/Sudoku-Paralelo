"""Leitura e validação de Sudokus (arquivo .txt ou dataset .csv).

Formato .txt:
  - 9 linhas de 9 dígitos por tabuleiro; 0 = célula vazia
  - tabuleiros separados por linhas em branco
  - linhas iniciadas por '#' são comentários (o comentário imediatamente
    antes de um tabuleiro vira o nome dele)

Formato .csv (dataset "1 million Sudoku games"):
  - cabeçalho com as colunas `quizzes` e (opcional) `solutions`
  - cada célula é uma string de 81 dígitos, lida linha a linha
    (0 = vazio); `solutions` vira o `gabarito` do tabuleiro
"""
import csv
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple

TAMANHO = 9
DIGITOS = "0123456789"
QUANTIDADE_PADRAO = 50


class SudokuFormatError(ValueError):
    """Entrada malformada ou com pistas conflitantes."""


@dataclass
class Sudoku:
    id: int                      # posição no lote, começando em 1
    nome: str
    tabuleiro: List[List[int]]   # matriz 9x9, 0 = vazio
    gabarito: Optional[List[List[int]]] = None  # solução conhecida (CSV)


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


# ----------------------------------------------------------------------
# Formato .txt
# ----------------------------------------------------------------------
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


# ----------------------------------------------------------------------
# Formato .csv (dataset com 1 milhão de Sudokus)
# ----------------------------------------------------------------------
def _string_para_matriz(s: str, rotulo: str) -> List[List[int]]:
    """Converte 81 dígitos em matriz 9x9 (linha a linha)."""
    s = s.strip()
    if len(s) != TAMANHO * TAMANHO:
        raise SudokuFormatError(
            f"{rotulo}: esperados 81 dígitos, encontrados {len(s)}")
    if any(c not in DIGITOS for c in s):
        raise SudokuFormatError(f"{rotulo}: só são permitidos dígitos de 0 a 9")
    return [[int(c) for c in s[i * TAMANHO:(i + 1) * TAMANHO]]
            for i in range(TAMANHO)]


def ler_csv(caminho: str, quantidade: int, inicio: int = 0) -> List[Sudoku]:
    """Lê `quantidade` Sudokus do CSV, pulando as `inicio` primeiras linhas
    de dados. Lê em fluxo e para ao atingir a quantidade pedida, então
    funciona bem mesmo com o arquivo de 1 milhão de linhas.
    """
    if quantidade <= 0:
        raise SudokuFormatError("a quantidade deve ser um inteiro positivo")
    if inicio < 0:
        raise SudokuFormatError("o início não pode ser negativo")

    sudokus: List[Sudoku] = []
    with open(caminho, newline="", encoding="utf-8") as f:
        leitor = csv.reader(f)
        cabecalho = next(leitor, None)
        if cabecalho is None:
            raise SudokuFormatError("CSV vazio")
        cols = [c.strip().lower() for c in cabecalho]
        if "quizzes" not in cols:
            raise SudokuFormatError(
                f"coluna 'quizzes' não encontrada (colunas: {cabecalho})")
        iq = cols.index("quizzes")
        isol = cols.index("solutions") if "solutions" in cols else None

        for n, linha in enumerate(leitor):      # n = índice da linha de dados
            if n < inicio:
                continue
            if len(sudokus) >= quantidade:
                break
            rotulo = f"CSV linha de dados {n + 1}"
            try:
                tab = _string_para_matriz(linha[iq], rotulo)
                gab = (_string_para_matriz(linha[isol], rotulo + " (solução)")
                       if isol is not None else None)
            except IndexError:
                raise SudokuFormatError(f"{rotulo}: linha com colunas faltando")
            conflitos = encontrar_conflitos(tab)
            if conflitos:
                raise SudokuFormatError(
                    f"{rotulo}: conflito nas pistas: " + "; ".join(conflitos))
            sudokus.append(Sudoku(len(sudokus) + 1, f"csv_{n + 1}", tab, gab))

    if not sudokus:
        raise SudokuFormatError(
            f"nenhum Sudoku lido (início={inicio} está além do fim do arquivo?)")
    return sudokus


# ----------------------------------------------------------------------
# Ponto de entrada único: escolhe o leitor pela extensão
# ----------------------------------------------------------------------
def perguntar_quantidade(padrao: int = QUANTIDADE_PADRAO) -> int:
    """Pergunta no terminal quantos Sudokus ler. Enter = padrão."""
    while True:
        try:
            resp = input(f"Quantos Sudokus ler do CSV? [{padrao}]: ").strip()
        except EOFError:        # sem terminal interativo (ex.: redirecionado)
            return padrao
        if resp == "":
            return padrao
        if resp.isdigit() and int(resp) > 0:
            return int(resp)
        print("Digite um número inteiro positivo.")


def carregar(caminho: str, quantidade: Optional[int] = None,
             inicio: int = 0) -> List[Sudoku]:
    """Carrega Sudokus de .txt ou .csv.

    - .txt: lê todos (quantidade/inicio são ignorados).
    - .csv: se `quantidade` for None, pergunta no terminal.
    """
    if caminho.lower().endswith(".csv"):
        if quantidade is None:
            quantidade = perguntar_quantidade()
        return ler_csv(caminho, quantidade, inicio)
    return ler_sudokus(caminho)


if __name__ == "__main__":
    caminho = sys.argv[1] if len(sys.argv) > 1 else "dados/sudokus.txt"
    try:
        sudokus = carregar(caminho)
    except (OSError, SudokuFormatError) as e:
        print(f"ERRO: {e}")
        sys.exit(1)
    print(f"{len(sudokus)} Sudoku(s) lido(s) de {caminho}\n")
    for s in sudokus[:5]:
        pistas = sum(1 for linha in s.tabuleiro for v in linha if v)
        print(f"[{s.id}] {s.nome} — {pistas} pistas")
        for linha in s.tabuleiro:
            print("  " + " ".join(str(v) if v else "." for v in linha))
        print()
    if len(sudokus) > 5:
        print(f"... (mostrando só os 5 primeiros de {len(sudokus)})")
