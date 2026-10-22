/*
 * ICC002 — Algoritmos e Estruturas de Dados I — Trabalho 1
 * Análise da Qualidade do Ar e Clima de Manaus através de Estatísticas de Ordem
 *
 * Uso:
 *   ordem_estatistica [arquivo.csv] [-n N] [-m M] [-s semente] [-o saida.csv]
 *
 *   arquivo.csv  medições reais (padrão: dados/dados_clima_manaus.csv)
 *   -n N         tamanho mínimo do conjunto (padrão 10000). Se o arquivo
 *                tiver menos registros, o restante é simulado.
 *   -m M         quantidade de dias mais secos a listar (padrão 10)
 *   -s semente   semente da simulação e do pivô aleatório (padrão 2026)
 *   -o arquivo   grava o conjunto efetivamente usado (padrão
 *                resultados/dados_utilizados.csv; "-" para não gravar)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "experimentos.h"
#include "medicao.h"

#define N_MINIMO_ESPECIFICACAO 10000

static void uso(const char *prog)
{
    fprintf(stderr,
            "Uso: %s [arquivo.csv] [-n N] [-m M] [-s semente] [-o saida.csv]\n",
            prog);
    exit(EXIT_FAILURE);
}

static long ler_numero(const char *s, const char *prog)
{
    char *fim;
    long v = strtol(s, &fim, 10);
    if (*s == '\0' || *fim != '\0' || v < 0)
        uso(prog);
    return v;
}

int main(int argc, char *argv[])
{
    const char *entrada = "dados/dados_clima_manaus.csv";
    const char *saida = "resultados/dados_utilizados.csv";
    long n_desejado = N_MINIMO_ESPECIFICACAO, m = 10;
    uint64_t semente = 2026;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            n_desejado = ler_numero(argv[++i], argv[0]);
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            m = ler_numero(argv[++i], argv[0]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
            semente = (uint64_t)ler_numero(argv[++i], argv[0]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            saida = argv[++i];
        else if (argv[i][0] == '-')
            uso(argv[0]);
        else
            entrada = argv[i];
    }
    if (m < 1 || n_desejado > 2000000)
        uso(argv[0]);

    /* ---- 1. Entrada dos dados ------------------------------------------ */
    Conjunto c;
    conjunto_iniciar(&c);
    size_t invalidas = 0;
    if (ler_csv(entrada, &c, &invalidas) != 0) {
        fprintf(stderr, "Erro: não foi possível ler '%s'.\n", entrada);
        return EXIT_FAILURE;
    }
    size_t reais = c.qtd;

    printf("================================================================"
           "================\n"
           "1. DADOS DE ENTRADA\n"
           "================================================================"
           "================\n");
    printf("Arquivo: %s\n", entrada);
    printf("Registros válidos lidos: %zu   (linhas descartadas: %zu)\n", reais,
           invalidas);

    if (c.qtd < (size_t)n_desejado) {
        size_t faltam = (size_t)n_desejado - c.qtd;
        if (conjunto_reservar(&c, (size_t)n_desejado) != 0) {
            fprintf(stderr, "Erro: memória insuficiente.\n");
            return EXIT_FAILURE;
        }
        Rng rng;
        rng_semear(&rng, semente);
        simular_medicoes(c.itens + c.qtd, faltam, &rng);
        c.qtd += faltam;
        printf("Simulados: %zu registros do verão amazônico (ago-out), semente %llu,\n"
               "para atingir N = %ld (a especificação exige N >= %d).\n",
               faltam, (unsigned long long)semente, n_desejado,
               N_MINIMO_ESPECIFICACAO);
    }
    int n = (int)c.qtd;
    if (n == 0) {
        fprintf(stderr, "Erro: nenhum registro para processar.\n");
        return EXIT_FAILURE;
    }
    printf("Total processado: N = %d registros (%zu reais + %zu simulados)\n", n,
           reais, c.qtd - reais);
    if (strcmp(saida, "-") != 0) {
        if (gravar_csv(saida, c.itens, c.qtd) == 0)
            printf("Conjunto utilizado gravado em: %s\n", saida);
        else
            fprintf(stderr, "Aviso: não foi possível gravar '%s'.\n", saida);
    }

    /* ---- 2 a 5. Consultas, desempenho, casos e questões ---------------- */
    executar_consultas(c.itens, n, (int)m);
    analisar_desempenho(c.itens, n, (int)m);
    analisar_casos(1000, semente);
    analisar_casos(10000, semente);
    questao1_rastreio_contagem();
    questao2_empates();
    questao3_escolha_do_pivo(c.itens, n < 10000 ? n : 10000, semente);
    questao5_reutilizacao(c.itens, n);

    conjunto_liberar(&c);
    return EXIT_SUCCESS;
}
