CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDLIBS  := -lm

SRC_LIB := src/aleatorio.c src/medicao.c src/ordem_estatistica.c
SRC_APP := $(SRC_LIB) src/experimentos.c src/main.c
HEADERS := $(wildcard src/*.h)

PROG    := build/ordem_estatistica
TESTES  := build/testes

.PHONY: all run test clean

all: $(PROG)

$(PROG): $(SRC_APP) $(HEADERS) | build
	$(CC) $(CFLAGS) -o $@ $(SRC_APP) $(LDLIBS)

$(TESTES): tests/testes.c $(SRC_LIB) $(HEADERS) | build
	$(CC) $(CFLAGS) -Isrc -o $@ tests/testes.c $(SRC_LIB) $(LDLIBS)

build resultados:
	mkdir -p $@

# Executa sobre o CSV fornecido e guarda a saída completa em resultados/.
run: $(PROG) | resultados
	./$(PROG) dados/dados_clima_manaus.csv | tee resultados/saida.txt

test: $(TESTES)
	./$(TESTES)

clean:
	rm -rf build
