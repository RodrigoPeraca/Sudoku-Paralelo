/* core.c - backtracking com MRV, máscaras de bits e pilha explícita.
 *
 * Mesma regra do protótipo em Python: a célula escolhida é a primeira
 * (em ordem linha a linha) com o menor número de candidatos, e os
 * valores são tentados em ordem crescente. Por isso o número de nós
 * coincide com o da versão Python.
 */
#include "sudoku.h"
#include <string.h>

#define TODOS 0x3FEu /* bits 1..9 */

typedef struct {
    uint8_t  b[N_CELULAS];
    uint16_t row[9], col[9], box[9]; /* bit v ligado = valor v já usado */
} Estado;

typedef struct {
    uint16_t cands; /* candidatos ainda não tentados */
    uint16_t bit;   /* valor atualmente colocado (0 = nenhum) */
    uint8_t  cell;
} Frame;

static inline void colocar(Estado *e, int cell, uint16_t bit)
{
    int r = cell / 9, c = cell % 9, bx = (r / 3) * 3 + c / 3;
    e->b[cell] = (uint8_t)__builtin_ctz(bit);
    e->row[r] |= bit;
    e->col[c] |= bit;
    e->box[bx] |= bit;
}

static inline void remover(Estado *e, int cell, uint16_t bit)
{
    int r = cell / 9, c = cell % 9, bx = (r / 3) * 3 + c / 3;
    e->b[cell] = 0;
    e->row[r] &= (uint16_t)~bit;
    e->col[c] &= (uint16_t)~bit;
    e->box[bx] &= (uint16_t)~bit;
}

/* MRV: devolve 0 se não há célula vazia; senão preenche cell/cands. */
static int escolher(const Estado *e, int *cell, uint16_t *cands)
{
    int melhor = -1, melhor_n = 10;
    uint16_t melhor_c = 0;
    for (int i = 0; i < N_CELULAS; i++) {
        if (e->b[i])
            continue;
        int r = i / 9, c = i % 9, bx = (r / 3) * 3 + c / 3;
        uint16_t cd = (uint16_t)(TODOS & ~(e->row[r] | e->col[c] | e->box[bx]));
        int n = __builtin_popcount(cd);
        if (n < melhor_n) {
            melhor = i;
            melhor_n = n;
            melhor_c = cd;
            if (n == 0)
                break; /* beco sem saída: não adianta procurar mais */
        }
    }
    if (melhor < 0)
        return 0;
    *cell = melhor;
    *cands = melhor_c;
    return 1;
}

StatusSolver sudoku_resolver(const uint8_t *entrada, uint8_t *saida,
                             uint64_t *nos)
{
    Estado e;
    Frame st[N_CELULAS];
    int depth = 0;
    uint64_t cont = 0;

    memset(&e, 0, sizeof e);
    if (nos)
        *nos = 0;

    for (int i = 0; i < N_CELULAS; i++) {
        uint8_t v = entrada[i];
        if (v == 0)
            continue;
        if (v > 9)
            return RES_CONFLITO;
        uint16_t bit = (uint16_t)(1u << v);
        int r = i / 9, c = i % 9, bx = (r / 3) * 3 + c / 3;
        if ((e.row[r] | e.col[c] | e.box[bx]) & bit)
            return RES_CONFLITO;
        colocar(&e, i, bit);
    }

    for (;;) {
        int cell;
        uint16_t cands;

        if (!escolher(&e, &cell, &cands))
            break; /* nenhuma célula vazia: resolvido */

        st[depth].cell = (uint8_t)cell;
        st[depth].cands = cands;
        st[depth].bit = 0;
        depth++;

        for (;;) { /* tenta o próximo candidato do topo; senão, retrocede */
            Frame *f = &st[depth - 1];
            if (f->bit) {
                remover(&e, f->cell, f->bit);
                f->bit = 0;
            }
            if (!f->cands) {
                depth--;
                if (depth == 0) {
                    if (nos)
                        *nos = cont;
                    return RES_SEM_SOLUCAO;
                }
                continue;
            }
            uint16_t bit = (uint16_t)(f->cands & -f->cands); /* menor bit */
            f->cands ^= bit;
            f->bit = bit;
            colocar(&e, f->cell, bit);
            cont++;
            break;
        }
    }

    memcpy(saida, e.b, N_CELULAS);
    if (nos)
        *nos = cont;
    return RES_OK;
}