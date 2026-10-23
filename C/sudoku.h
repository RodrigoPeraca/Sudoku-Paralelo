/* sudoku.h - núcleo do solucionador de Sudoku em C (sequencial).
 *
 * Tabuleiro: vetor de 81 bytes (linha a linha), 0 = célula vazia.
 * O núcleo não conhece paralelismo: os drivers (OpenMP, pthreads, MPI,
 * GPU) só distribuem tabuleiros e chamam sudoku_resolver().
 */
#ifndef SUDOKU_H
#define SUDOKU_H

#include <stddef.h>
#include <stdint.h>

#define N_CELULAS 81

typedef struct {
    int     id;              /* posição no lote (1..n)               */
    long    linha;           /* linha de dados no arquivo (1..)      */
    uint8_t tab[N_CELULAS];  /* pistas (0 = vazio)                   */
    uint8_t gab[N_CELULAS];  /* solução conhecida, se tem_gab        */
    int     tem_gab;
} Puzzle;

typedef enum {
    RES_OK = 0,
    RES_SEM_SOLUCAO = 1,
    RES_CONFLITO = 2         /* pistas iniciais se contradizem */
} StatusSolver;

/* Resolve `entrada` (não é modificada) e escreve a solução em `saida`.
 * `nos` (opcional) recebe o número de valores tentados. */
StatusSolver sudoku_resolver(const uint8_t *entrada, uint8_t *saida,
                             uint64_t *nos);

/* Validação independente: 1..9 sem repetição em linhas, colunas e blocos
 * e, se `original` != NULL, pistas originais preservadas. 1 = válida. */
int sudoku_valido(const uint8_t *sol, const uint8_t *original);

/* Lê `quantidade` Sudokus do CSV (colunas quizzes[,solutions]), pulando
 * as `inicio` primeiras linhas de dados. Devolve 0 em sucesso (e aloca
 * *out, que o chamador libera com free) ou -1 com mensagem em `erro`. */
int carregar_csv(const char *caminho, long inicio, int quantidade,
                 Puzzle **out, int *n, char *erro, size_t erro_tam);

#endif