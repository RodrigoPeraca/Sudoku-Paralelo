/* main_omp.c - driver OpenMP: cada thread resolve Sudokus inteiros e
 * independentes (paralelismo por replicação).
 *
 * O laço sobre o lote é distribuído entre as threads. Cada iteração:
 *   - usa só variáveis locais (sudoku_resolver não tem estado global);
 *   - grava o resultado em res[i], posição exclusiva da iteração,
 *     então não há corrida de dados nem necessidade de trava.
 * A impressão e a soma das estatísticas acontecem depois, em um único
 * fluxo, para a saída ficar em ordem e determinística.
 */
#include "sudoku.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUANTIDADE_PADRAO 50
#define CHUNK_MAX 64

typedef struct {
    uint64_t nos;
    double   tempo;   /* tempo só do solucionador, em segundos */
    int      thread;  /* thread que resolveu este Sudoku        */
    uint8_t  status, valido, tem_gab, igual;
} Resultado;

static int perguntar_quantidade(void)
{
    char buf[64];
    for (;;) {
        printf("Quantos Sudokus ler do CSV? [%d]: ", QUANTIDADE_PADRAO);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin))
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
        "uso: %s arquivo.csv [-n QUANT] [--inicio K] [-t THREADS]\n"
        "        [--sched static|dynamic|guided] [--chunk C] [-v]\n"
        "  -n        quantos Sudokus ler (se omitido, pergunta)\n"
        "  --inicio  pular as K primeiras linhas de dados\n"
        "  -t        número de threads (padrão: OMP_NUM_THREADS ou nº de núcleos)\n"
        "  --sched   política de distribuição das iterações (padrão: dynamic)\n"
        "  --chunk   tamanho do bloco de iterações. Padrão: automático para\n"
        "            dynamic/guided (n/(8*threads), entre 1 e %d); static usa\n"
        "            blocos iguais\n"
        "  -v        imprimir uma linha por Sudoku\n",
        prog, CHUNK_MAX);
}

static const char *nome_sched(omp_sched_t k)
{
    switch ((int)k & 0x7) {
    case 1: return "static";
    case 2: return "dynamic";
    case 3: return "guided";
    default: return "auto";
    }
}

int main(int argc, char **argv)
{
    const char *caminho = NULL;
    int quantidade = 0, verbose = 0, threads = 0, chunk = 0;
    int sched_dado = 0;
    omp_sched_t kind = omp_sched_dynamic;
    long inicio = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc)
            quantidade = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--inicio") && i + 1 < argc)
            inicio = atol(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc)
            threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--chunk") && i + 1 < argc) {
            chunk = atoi(argv[++i]);
            sched_dado = 1;
        } else if (!strcmp(argv[i], "--sched") && i + 1 < argc) {
            const char *s = argv[++i];
            if (!strcmp(s, "static"))       kind = omp_sched_static;
            else if (!strcmp(s, "dynamic")) kind = omp_sched_dynamic;
            else if (!strcmp(s, "guided"))  kind = omp_sched_guided;
            else { uso(argv[0]); return 2; }
            sched_dado = 1;
        } else if (!strcmp(argv[i], "-v"))
            verbose = 1;
        else if (argv[i][0] != '-' && !caminho)
            caminho = argv[i];
        else { uso(argv[0]); return 2; }
    }
    if (!caminho || chunk < 0 || threads < 0) { uso(argv[0]); return 2; }
    if (quantidade <= 0)
        quantidade = perguntar_quantidade();

    if (threads > 0)
        omp_set_num_threads(threads);

    Puzzle *lote = NULL;
    int n = 0;
    char erro[256];
    if (carregar_csv(caminho, inicio, quantidade, &lote, &n, erro,
                     sizeof erro) != 0) {
        fprintf(stderr, "ERRO: %s\n", erro);
        return 1;
    }

    int nt = omp_get_max_threads();

    /* Política de distribuição. Se o usuário definiu OMP_SCHEDULE no
     * ambiente e não passou --sched/--chunk, respeita a variável.
     * Com chunk automático, lotes pequenos ainda se dividem entre as
     * threads (um chunk fixo grande deixaria quase tudo em uma thread). */
    if (sched_dado || getenv("OMP_SCHEDULE") == NULL) {
        if (chunk == 0 && kind != omp_sched_static) {
            chunk = n / (8 * nt);
            if (chunk < 1) chunk = 1;
            if (chunk > CHUNK_MAX) chunk = CHUNK_MAX;
        }
        omp_set_schedule(kind, chunk);
    }
    omp_sched_t sk; int sc;
    omp_get_schedule(&sk, &sc);
    printf("%d Sudoku(s) carregado(s) de %s\n", n, caminho);
    printf("OpenMP: %d thread(s), schedule=%s, chunk=%d (núcleos visíveis: %d)\n\n",
           nt, nome_sched(sk), sc, omp_get_num_procs());

    Resultado *res = calloc((size_t)n, sizeof *res);
    if (!res) { fprintf(stderr, "ERRO: sem memória\n"); return 1; }

    double t_total = omp_get_wtime();

    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < n; i++) {
        uint8_t sol[N_CELULAS];
        uint64_t nos = 0;
        double t0 = omp_get_wtime();
        StatusSolver st = sudoku_resolver(lote[i].tab, sol, &nos);
        double dt = omp_get_wtime() - t0;

        Resultado *r = &res[i];
        r->nos = nos;
        r->tempo = dt;
        r->thread = omp_get_thread_num();
        r->status = (uint8_t)st;
        r->valido = (uint8_t)(st == RES_OK && sudoku_valido(sol, lote[i].tab));
        r->tem_gab = (uint8_t)lote[i].tem_gab;
        r->igual = (uint8_t)(st == RES_OK && lote[i].tem_gab &&
                             memcmp(sol, lote[i].gab, N_CELULAS) == 0);
    }

    t_total = omp_get_wtime() - t_total;

    /* ---- a partir daqui, tudo sequencial ---- */
    int resolvidos = 0, validos = 0, sem_sol = 0, conflitos = 0;
    int com_gab = 0, iguais = 0;
    uint64_t nos_total = 0;
    double maximo = 0;
    long *por_thread = calloc((size_t)nt, sizeof *por_thread);
    double *ocupado = calloc((size_t)nt, sizeof *ocupado);

    for (int i = 0; i < n; i++) {
        const Resultado *r = &res[i];
        nos_total += r->nos;
        if (r->tempo > maximo) maximo = r->tempo;
        if (r->status == RES_OK) resolvidos++;
        else if (r->status == RES_SEM_SOLUCAO) sem_sol++;
        else conflitos++;
        validos += r->valido;
        com_gab += r->tem_gab && r->status == RES_OK;
        iguais += r->igual;
        if (por_thread && r->thread < nt) {
            por_thread[r->thread]++;
            ocupado[r->thread] += r->tempo;
        }
        if (verbose) {
            const char *rot = r->status == RES_OK
                ? (r->valido ? "RESOLVIDO e VÁLIDO" : "INVÁLIDO")
                : (r->status == RES_SEM_SOLUCAO ? "NÃO RESOLVIDO (sem solução)"
                                                : "NÃO RESOLVIDO (conflito nas pistas)");
            printf("[%d] linha %ld: %s | nós=%llu | %.6fs | thread %d\n",
                   lote[i].id, lote[i].linha, rot,
                   (unsigned long long)r->nos, r->tempo, r->thread);
        }
    }

    printf("%s--- Resumo ---\n", verbose ? "\n" : "");
    printf("Sudokus: %d | resolvidos: %d | válidos: %d | sem solução: %d | "
           "conflito: %d\n", n, resolvidos, validos, sem_sol, conflitos);
    if (com_gab)
        printf("Iguais ao gabarito do CSV: %d/%d\n", iguais, com_gab);
    printf("Nós (valores tentados) no total: %llu\n",
           (unsigned long long)nos_total);
    printf("Tempo total (parede): %.6fs | máximo de um Sudoku: %.6fs\n",
           t_total, maximo);

    if (por_thread && nt <= 16) {
        printf("\nThread | Sudokus | tempo resolvendo\n");
        for (int t = 0; t < nt; t++)
            printf("%6d | %7ld | %.6fs\n", t, por_thread[t], ocupado[t]);
    }

    free(por_thread);
    free(ocupado);
    free(res);
    free(lote);
    return 0;
}