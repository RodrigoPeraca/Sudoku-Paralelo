/* io.c - leitura do CSV (colunas quizzes[,solutions], 81 dígitos cada). */
#include "sudoku.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO(...)                                          \
    do {                                                   \
        if (erro && erro_tam)                              \
            snprintf(erro, erro_tam, __VA_ARGS__);         \
    } while (0)

static int parse81(const char *s, uint8_t *out)
{
    if (strlen(s) != N_CELULAS)
        return 0;
    for (int i = 0; i < N_CELULAS; i++) {
        if (s[i] < '0' || s[i] > '9')
            return 0;
        out[i] = (uint8_t)(s[i] - '0');
    }
    return 1;
}

int carregar_csv(const char *caminho, long inicio, int quantidade,
                 Puzzle **out, int *n, char *erro, size_t erro_tam)
{
    char linha[1024];
    long dado = 0; /* número da linha de dados (1-based) */
    int lidos = 0;
    Puzzle *v;
    FILE *f;

    if (quantidade <= 0 || inicio < 0) {
        ERRO("quantidade deve ser > 0 e início >= 0");
        return -1;
    }
    f = fopen(caminho, "r");
    if (!f) {
        ERRO("não foi possível abrir '%s'", caminho);
        return -1;
    }
    if (!fgets(linha, sizeof linha, f) || strncmp(linha, "quizzes", 7) != 0) {
        fclose(f);
        ERRO("cabeçalho inválido: esperado 'quizzes' (ou 'quizzes,solutions')");
        return -1;
    }
    v = malloc((size_t)quantidade * sizeof *v);
    if (!v) {
        fclose(f);
        ERRO("sem memória para %d Sudokus", quantidade);
        return -1;
    }

    while (lidos < quantidade && fgets(linha, sizeof linha, f)) {
        char *virg, *segundo = NULL;
        size_t len = strlen(linha);

        while (len && (linha[len - 1] == '\n' || linha[len - 1] == '\r'))
            linha[--len] = '\0';
        if (len == 0)
            continue; /* linha em branco */
        dado++;
        if (dado <= inicio)
            continue;

        virg = strchr(linha, ',');
        if (virg) {
            *virg = '\0';
            segundo = virg + 1;
        }
        Puzzle *p = &v[lidos];
        if (!parse81(linha, p->tab)) {
            free(v);
            fclose(f);
            ERRO("linha de dados %ld: 'quizzes' deve ter 81 dígitos (0-9)", dado);
            return -1;
        }
        p->tem_gab = (segundo && *segundo && parse81(segundo, p->gab));
        p->id = lidos + 1;
        p->linha = dado;
        lidos++;
    }
    fclose(f);

    if (lidos == 0) {
        free(v);
        ERRO("nenhum Sudoku lido (início=%ld está além do fim do arquivo?)",
             inicio);
        return -1;
    }
    *out = v;
    *n = lidos;
    return 0;
}