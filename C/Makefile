CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=gnu11
NUCLEO  = core.c validar.c io.c

all: sudoku_seq sudoku_omp

sudoku_seq: main_seq.c $(NUCLEO) sudoku.h
	$(CC) $(CFLAGS) -o $@ main_seq.c $(NUCLEO)

sudoku_omp: main_omp.c $(NUCLEO) sudoku.h
	$(CC) $(CFLAGS) -fopenmp -o $@ main_omp.c $(NUCLEO)

test_core: test_core.c core.c validar.c sudoku.h
	$(CC) $(CFLAGS) -o $@ test_core.c core.c validar.c

test: test_core
	./test_core

check: all
	./verificar_omp.sh

clean:
	rm -f sudoku_seq sudoku_omp test_core

.PHONY: all test check clean