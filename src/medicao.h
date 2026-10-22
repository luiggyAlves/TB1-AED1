#ifndef MEDICAO_H
#define MEDICAO_H

#include <stddef.h>

#include "aleatorio.h"

#define TAM_DATA 11  /* "AAAA-MM-DD" + '\0' */

/* Um registro diário de um sensor, exatamente como no CSV de entrada. */
typedef struct {
    int   id_sensor;
    char  data[TAM_DATA];
    float temperatura_c;
    int   umidade_porcentagem;
    float nivel_pm25;          /* µg/m³ */
} Medicao;

/* Campo usado como chave de comparação pelos algoritmos de seleção. */
typedef enum {
    CAMPO_TEMPERATURA,
    CAMPO_UMIDADE,
    CAMPO_PM25
} Campo;

double      medicao_chave(const Medicao *m, Campo campo);
const char *campo_nome(Campo campo);

/* Vetor dinâmico de medições. */
typedef struct {
    Medicao *itens;
    size_t   qtd;
    size_t   capacidade;
} Conjunto;

void conjunto_iniciar(Conjunto *c);
int  conjunto_adicionar(Conjunto *c, const Medicao *m);
int  conjunto_reservar(Conjunto *c, size_t capacidade);
void conjunto_liberar(Conjunto *c);

/*
 * Lê um CSV no formato
 *   ID_Sensor,Data,Temperatura_C,Umidade_Porcentagem,Nivel_PM25
 * acrescentando os registros válidos em `c`. Aceita finais de linha LF ou
 * CRLF e cabeçalho opcional. Linhas malformadas são descartadas e contadas
 * em `*invalidas`. Retorna 0 em caso de sucesso ou -1 se o arquivo não
 * puder ser aberto.
 */
int ler_csv(const char *caminho, Conjunto *c, size_t *invalidas);

/* Grava as medições no mesmo formato CSV da entrada. Retorna 0 ou -1. */
int gravar_csv(const char *caminho, const Medicao *v, size_t n);

/*
 * Simula `qtd` medições do "verão amazônico" (agosto a outubro) em Manaus:
 * temperatura alta, umidade baixa e PM2.5 elevado nos picos de queimadas
 * ("fumaceiro"), com umidade inversamente relacionada à temperatura.
 */
void simular_medicoes(Medicao *destino, size_t qtd, Rng *rng);

#endif
