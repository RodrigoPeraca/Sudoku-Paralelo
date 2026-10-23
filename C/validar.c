/* validar.c - verificação independente de soluções (lógica própria,
 * sem usar nada de core.c). */
#include "sudoku.h"

int sudoku_valido(const uint8_t *sol, const uint8_t *original)
{
    uint16_t row[9] = {0}, col[9] = {0}, box[9] = {0};

    for (int i = 0; i < N_CELULAS; i++) {
        if (sol[i] < 1 || sol[i] > 9)
            return 0;
        if (original && original[i] && original[i] != sol[i])
            return 0; /* pista alterada */
        uint16_t bit = (uint16_t)(1u << sol[i]);
        int r = i / 9, c = i % 9;
        row[r] |= bit;
        col[c] |= bit;
        box[(r / 3) * 3 + c / 3] |= bit;
    }
    /* 9 células por unidade: união == 1..9 significa sem repetição */
    for (int u = 0; u < 9; u++)
        if (row[u] != 0x3FE || col[u] != 0x3FE || box[u] != 0x3FE)
            return 0;
    return 1;
}