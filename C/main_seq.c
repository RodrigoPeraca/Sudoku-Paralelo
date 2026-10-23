/* main_seq.c - driver sequencial: lê o CSV, resolve tudo em ordem,
 * valida e mede o tempo. Referência T(1) para os drivers paralelos. */
#define _POSIX_C_SOURCE 199309L
#include "sudoku.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QUANTIDADE_PADRAO 50

static double agora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static int perguntar_quantidade(void)
{
    char buf[64];
    for (;;) {
        printf("Quantos Sudokus ler do CSV? [%d]: ", QUANTIDADE_PADRAO);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) /* sem terminal: usa o padrão */
            return QUANTIDADE_PADRAO;
        if (buf[0] == '\n')
            return QUANTIDADE_PADRAO;
        char *fim;
        long q = strtol(buf, &fim, 10);
        if (fim != buf && q > 0 && q <= 100000000L)
            return (int)q;
        printf("Digite um número inteiro positivo.\n");
    }
}

static void uso(const char *prog)
{
    fprintf(stderr,
            "uso: %s arquivo.csv [-n QUANTIDADE] [--inicio K] [-v]\n"
            "  -n        quantos Sudokus ler (se omitido, pergunta)\n"
            "  --inicio  pular as K primeiras linhas de dados\n"
            "  -v        imprimir uma linha por Sudoku\n",
            prog);
}

int main(int argc, char **argv)
{
    const char *caminho = NULL;
    int quantidade = 0, verbose = 0;
    long inicio = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc)
            quantidade = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--inicio") && i + 1 < argc)
            inicio = atol(argv[++i]);
        else if (!strcmp(argv[i], "-v"))
            verbose = 1;
        else if (argv[i][0] != '-' && !caminho)
            caminho = argv[i];
        else {
            uso(argv[0]);
            return 2;
        }
    }
    if (!caminho) {
        uso(argv[0]);
        return 2;
    }
    if (quantidade <= 0)
        quantidade = perguntar_quantidade();

    Puzzle *lote = NULL;
    int n = 0;
    char erro[256];
    if (carregar_csv(caminho, inicio, quantidade, &lote, &n, erro,
                     sizeof erro) != 0) {
        fprintf(stderr, "ERRO: %s\n", erro);
        return 1;
    }
    printf("%d Sudoku(s) carregado(s) de %s\n\n", n, caminho);

    double *tempos = malloc((size_t)n * sizeof *tempos);
    int resolvidos = 0, validos = 0, sem_sol = 0, conflitos = 0;
    int com_gab = 0, iguais = 0;
    uint64_t nos_total = 0;

    double t_total = agora();
    for (int i = 0; i < n; i++) {
        uint8_t sol[N_CELULAS];
        uint64_t nos = 0;
        double t0 = agora();
        StatusSolver st = sudoku_resolver(lote[i].tab, sol, &nos);
        tempos[i] = agora() - t0;
        nos_total += nos;

        const char *rotulo;
        if (st == RES_OK) {
            resolvidos++;
            int ok = sudoku_valido(sol, lote[i].tab);
            validos += ok;
            rotulo = ok ? "RESOLVIDO e VÁLIDO" : "INVÁLIDO";
            if (lote[i].tem_gab) {
                com_gab++;
                iguais += (memcmp(sol, lote[i].gab, N_CELULAS) == 0);
            }
        } else if (st == RES_SEM_SOLUCAO) {
            sem_sol++;
            rotulo = "NÃO RESOLVIDO (sem solução)";
        } else {
            conflitos++;
            rotulo = "NÃO RESOLVIDO (conflito nas pistas)";
        }
        if (verbose)
            printf("[%d] linha %ld: %s | nós=%llu | %.6fs\n", lote[i].id,
                   lote[i].linha, rotulo, (unsigned long long)nos, tempos[i]);
    }
    t_total = agora() - t_total;

    double maximo = 0;
    for (int i = 0; i < n; i++)
        if (tempos[i] > maximo)
            maximo = tempos[i];
    qsort(tempos, (size_t)n, sizeof *tempos, cmp_double);
    double mediana = (n % 2) ? tempos[n / 2]
                             : (tempos[n / 2 - 1] + tempos[n / 2]) / 2.0;

    printf("%s--- Resumo ---\n", verbose ? "\n" : "");
    printf("Sudokus: %d | resolvidos: %d | válidos: %d | sem solução: %d | "
           "conflito: %d\n", n, resolvidos, validos, sem_sol, conflitos);
    if (com_gab)
        printf("Iguais ao gabarito do CSV: %d/%d\n", iguais, com_gab);
    printf("Nós (valores tentados) no total: %llu\n",
           (unsigned long long)nos_total);
    printf("Tempo total: %.6fs | mediana: %.6fs | máximo: %.6fs\n", t_total,
           mediana, maximo);

    free(tempos);
    free(lote);
    return 0;
}