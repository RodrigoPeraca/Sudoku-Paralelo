/* test_core.c - testes do núcleo (sem arquivos). Saída 0 = tudo OK. */
#include "sudoku.h"
#include <stdio.h>
#include <string.h>

static int falhas = 0, total = 0;

#define CHECK(cond, nome)                                        \
    do {                                                         \
        total++;                                                 \
        if (cond) {                                              \
            printf("ok   - %s\n", nome);                         \
        } else {                                                 \
            printf("FALHA - %s\n", nome);                        \
            falhas++;                                            \
        }                                                        \
    } while (0)

static void ler(const char *s, uint8_t *tab)
{
    for (int i = 0; i < N_CELULAS; i++)
        tab[i] = (uint8_t)(s[i] - '0');
}

static const char *FACIL =
    "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
static const char *MEDIO =
    "003020600900305001001806400008102900700000008006708200002609500800203009005010300";
static const char *DIFICIL =
    "800000000003600000070090200050007000000045700000100030001000068008500010090000400";
static const char *RESOLVIDO =
    "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
static const char *SEM_SOLUCAO =
    "123456780000000009000000000000000000000000000000000000000000000000000000000000000";

/* Os números de nós esperados vêm da versão Python (mesma regra de MRV). */
static void caso_resolvivel(const char *nome, const char *s,
                            uint64_t nos_esperados)
{
    uint8_t tab[N_CELULAS], copia[N_CELULAS], sol[N_CELULAS];
    uint64_t nos = 0;
    char rot[96];

    ler(s, tab);
    memcpy(copia, tab, N_CELULAS);
    StatusSolver st = sudoku_resolver(tab, sol, &nos);

    snprintf(rot, sizeof rot, "%s: resolve", nome);
    CHECK(st == RES_OK, rot);
    snprintf(rot, sizeof rot, "%s: solução válida e pistas preservadas", nome);
    CHECK(st == RES_OK && sudoku_valido(sol, tab), rot);
    snprintf(rot, sizeof rot, "%s: nós = %llu (igual ao Python)", nome,
             (unsigned long long)nos_esperados);
    CHECK(nos == nos_esperados, rot);
    snprintf(rot, sizeof rot, "%s: entrada não modificada", nome);
    CHECK(memcmp(tab, copia, N_CELULAS) == 0, rot);
}

int main(void)
{
    uint8_t tab[N_CELULAS], sol[N_CELULAS];
    uint64_t nos;

    caso_resolvivel("facil", FACIL, 51);
    caso_resolvivel("medio", MEDIO, 49);
    caso_resolvivel("dificil", DIFICIL, 10101);
    caso_resolvivel("ja resolvido", RESOLVIDO, 0);

    ler(SEM_SOLUCAO, tab);
    CHECK(sudoku_resolver(tab, sol, &nos) == RES_SEM_SOLUCAO,
          "sem solução: reportado");

    ler(FACIL, tab);
    tab[2] = 5; /* 5 repetido na linha 1 */
    CHECK(sudoku_resolver(tab, sol, &nos) == RES_CONFLITO,
          "conflito nas pistas: reportado");

    /* validador */
    ler(RESOLVIDO, sol);
    CHECK(sudoku_valido(sol, NULL), "validador aceita solução correta");
    sol[0] = sol[1]; /* repetição na linha */
    CHECK(!sudoku_valido(sol, NULL), "validador rejeita repetição");
    ler(RESOLVIDO, sol);
    ler(FACIL, tab);
    uint8_t t = sol[0];
    sol[0] = sol[2];
    sol[2] = t; /* linha segue válida, mas a pista (0,0)=5 mudou */
    CHECK(!sudoku_valido(sol, tab), "validador rejeita pista alterada");
    sol[0] = 0;
    CHECK(!sudoku_valido(sol, NULL), "validador rejeita valor fora de 1..9");

    printf("\n%d testes, %d falha(s)\n", total, falhas);
    return falhas ? 1 : 0;
}