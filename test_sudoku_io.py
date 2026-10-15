import os
import unittest

from sudoku_io import (SudokuFormatError, encontrar_conflitos, ler_sudokus,
                       parse_sudokus)

FACIL = """\
530070000
600195000
098000060
800060003
400803001
700020006
060000280
000419005
000080079
"""

RESOLVIDO = """\
534678912
672195348
198342567
859761423
426853791
713924856
961537284
287419635
345286179
"""


class TestLeitura(unittest.TestCase):
    def test_le_dois_sudokus(self):
        s = parse_sudokus(FACIL + "\n\n" + RESOLVIDO)
        self.assertEqual(len(s), 2)
        self.assertEqual([x.id for x in s], [1, 2])

    def test_nome_vem_do_comentario(self):
        s = parse_sudokus("# meu teste\n" + FACIL + "\n" + RESOLVIDO)
        self.assertEqual(s[0].nome, "meu teste")
        self.assertEqual(s[1].nome, "sudoku_2")

    def test_valores_preservados(self):
        s = parse_sudokus(FACIL)[0]
        self.assertEqual(s.tabuleiro[0], [5, 3, 0, 0, 7, 0, 0, 0, 0])
        self.assertEqual(s.tabuleiro[8], [0, 0, 0, 0, 8, 0, 0, 7, 9])

    def test_arquivo_vazio(self):
        with self.assertRaises(SudokuFormatError):
            parse_sudokus("")

    def test_so_comentarios(self):
        with self.assertRaises(SudokuFormatError):
            parse_sudokus("# nada aqui\n\n# nem aqui\n")

    def test_bloco_com_poucas_linhas(self):
        with self.assertRaisesRegex(SudokuFormatError, "esperadas 9 linhas"):
            parse_sudokus("\n".join(FACIL.splitlines()[:8]))

    def test_linha_curta(self):
        texto = FACIL.replace("530070000", "53007000")
        with self.assertRaisesRegex(SudokuFormatError, "9 dígitos"):
            parse_sudokus(texto)

    def test_caractere_invalido(self):
        texto = FACIL.replace("530070000", "53007000x")
        with self.assertRaisesRegex(SudokuFormatError, "dígitos de 0 a 9"):
            parse_sudokus(texto)

    def test_conflito_em_linha(self):
        texto = FACIL.replace("530070000", "550070000")
        with self.assertRaisesRegex(SudokuFormatError, "linha 1"):
            parse_sudokus(texto)

    def test_conflito_em_bloco(self):
        # 8 em (2,2) e (3,3) do bloco 1, sem repetir linha nem coluna
        texto = FACIL.replace("600195000", "680195000")
        with self.assertRaisesRegex(SudokuFormatError, "bloco"):
            parse_sudokus(texto)

    def test_resolvido_sem_conflitos(self):
        s = parse_sudokus(RESOLVIDO)[0]
        self.assertEqual(encontrar_conflitos(s.tabuleiro), [])

    def test_arquivo_real(self):
        base = os.path.dirname(os.path.abspath(__file__))
        s = ler_sudokus(os.path.join(base, "dados", "sudokus.txt"))
        self.assertEqual(len(s), 2)


if __name__ == "__main__":
    unittest.main()
