#include "ordem_estatistica.h"

#include <stdlib.h>

void contexto_iniciar(Contexto *ctx, Campo campo)
{
    ctx->campo = campo;
    ctx->comparacoes = 0;
    ctx->pivo = PIVO_CENTRAL;
    ctx->rng = NULL;
    ctx->rastreio = NULL;
}

int comparar(const Medicao *a, const Medicao *b, Contexto *ctx)
{
    double x = medicao_chave(a, ctx->campo);
    double y = medicao_chave(b, ctx->campo);
    ctx->comparacoes++;
    return (x > y) - (x < y);
}

static void trocar(Medicao vetor[], int i, int k)
{
    Medicao elem = vetor[i];
    vetor[i] = vetor[k];
    vetor[k] = elem;
}

/* ======================================================================== */
/* Abordagem A — Ordenação completa (bolha) + acesso direto                 */
/* ======================================================================== */

void bolha(Medicao vetor[], int tam, Contexto *ctx)
{
    /* Igual ao código-base: tam-1 passadas completas, sem parada antecipada,
       portanto sempre tam*(tam-1)/2 comparações. */
    for (int j = 0; j < tam - 1; j++) {
        for (int i = 0; i < tam - 1 - j; i++) {
            if (comparar(&vetor[i], &vetor[i + 1], ctx) > 0) {
                trocar(vetor, i, i + 1);
            }
        }
    }
}

bool oe_ordenacao(Medicao vetor[], int tam, int ordem,
                  Medicao *e_valor_para_ordem, Contexto *ctx)
{
    if ((ordem < 1) || (ordem > tam)) {
        return false;
    }
    bolha(vetor, tam, ctx);

    *e_valor_para_ordem = vetor[ordem - 1];
    return true;
}

/* ======================================================================== */
/* Abordagem C — Contagem comparativa sequencial                            */
/* ======================================================================== */

/*
 * Compara `valor` com todos os elementos do vetor (inclusive ele mesmo,
 * como no código-base) e conta quantos são MENORES e quantos são MENORES
 * OU IGUAIS a ele.
 *
 * Correção para empates (Questão 2): o código-base contava apenas os
 * estritamente menores e aceitava o elemento quando menores + 1 == ordem.
 * Com valores repetidos, todas as cópias recebem a mesma "ordem"
 * (menores + 1) e as posições seguintes do grupo nunca são atingidas por
 * ninguém: o laço não encontra resposta e passa do fim do vetor. Contando
 * também os menores ou iguais, sabemos que o valor ocupa TODAS as posições
 * menores+1 .. menores_ou_iguais do vetor ordenado.
 */
static void contagem(Medicao vetor[], int tam, const Medicao *valor,
                     int *menores, int *menores_ou_iguais, Contexto *ctx)
{
    *menores = 0;
    *menores_ou_iguais = 0;
    for (int i = 0; i < tam; i++) {
        int c = comparar(&vetor[i], valor, ctx);
        if (c < 0) {
            (*menores)++;
        }
        if (c <= 0) {
            (*menores_ou_iguais)++;
        }
        if (ctx->rastreio != NULL) {
            fprintf(ctx->rastreio, "      %g comparado com %g: %s\n",
                    medicao_chave(&vetor[i], ctx->campo),
                    medicao_chave(valor, ctx->campo),
                    c < 0 ? "menor" : (c == 0 ? "igual" : "maior"));
        }
    }
}

bool oe_contagem(Medicao vetor[], int tam, int ordem,
                 Medicao *e_valor_para_ordem, Contexto *ctx)
{
    if ((ordem < 1) || (ordem > tam)) {
        return false;
    }
    int i = 0, menores, menores_ou_iguais;
    bool achei;
    do {
        if (ctx->rastreio != NULL) {
            fprintf(ctx->rastreio, "  Elemento %d (valor %g):\n", i + 1,
                    medicao_chave(&vetor[i], ctx->campo));
        }
        contagem(vetor, tam, &vetor[i], &menores, &menores_ou_iguais, ctx);
        achei = (menores < ordem) && (ordem <= menores_ou_iguais);
        if (ctx->rastreio != NULL) {
            fprintf(ctx->rastreio,
                    "    menores = %d, menores ou iguais = %d -> ocupa as "
                    "posições %d..%d; ordem %d %s\n",
                    menores, menores_ou_iguais, menores + 1, menores_ou_iguais,
                    ordem, achei ? "ESTÁ no intervalo: resposta encontrada"
                                 : "não está no intervalo");
        }
        i++;
    } while (!achei && i < tam);

    if (!achei) {
        return false;  /* inalcançável para 1 <= ordem <= tam */
    }
    *e_valor_para_ordem = vetor[i - 1];
    return true;
}

int listar_menores_contagem(Medicao vetor[], int tam, int m, Medicao saida[],
                            Contexto *ctx)
{
    if (m > tam) {
        m = tam;
    }
    int qtd = 0, na_fronteira = 0;
    for (int i = 0; i < tam && qtd < m; i++) {
        int menores, menores_ou_iguais;
        contagem(vetor, tam, &vetor[i], &menores, &menores_ou_iguais, ctx);

        if (menores_ou_iguais <= m) {
            /* Todas as posições do valor estão entre as m primeiras. */
            saida[qtd++] = vetor[i];
        } else if (menores < m && na_fronteira < m - menores) {
            /*
             * Valor repetido "na fronteira": só parte das cópias cabe nas m
             * primeiras posições. Todas as cópias têm o mesmo número de
             * menores, então sobram exatamente m - menores vagas para elas.
             */
            saida[qtd++] = vetor[i];
            na_fronteira++;
        }
    }
    return qtd;
}

/* ======================================================================== */
/* Abordagem B — Seleção por particionamento (divisão e conquista)          */
/* ======================================================================== */

static int escolher_pivo(int i_lim, int j_lim, Contexto *ctx)
{
    switch (ctx->pivo) {
    case PIVO_PRIMEIRO:
        return i_lim;
    case PIVO_ALEATORIO:
        return i_lim + (int)rng_intervalo(ctx->rng, (size_t)(j_lim - i_lim + 1));
    case PIVO_CENTRAL:
        break;
    }
    return (i_lim + j_lim) / 2;
}

/*
 * Particionamento com vetor espelho, como no código-base: percorre
 * vetor[i_lim..j_lim]; os menores que o pivô vão para o início de
 * v_espelho (posição i, que avança) e os maiores para o fim (posição j,
 * que recua). Ao final, as posições i..j ficam para o pivô.
 *
 * Correção para empates: o código-base só reservava UMA posição (j) para
 * o pivô, e os demais elementos iguais a ele não eram copiados para
 * nenhum lugar (sobrava "lixo" em v_espelho[i..j-1]). Aqui os iguais são
 * guardados à parte e todos eles preenchem o bloco i..j. Como os registros
 * iguais podem ser dias diferentes, guardamos os próprios registros (não
 * cópias do pivô): eles são compactados no trecho já lido de `vetor`,
 * sem comparações extras.
 *
 * Retorna j (última posição do bloco de iguais ao pivô) e devolve em
 * *inicio_iguais a primeira posição (i). Uma comparação por elemento.
 */
static int particionamento(Medicao vetor[], Medicao v_espelho[], int i_lim,
                           int j_lim, int *inicio_iguais, Contexto *ctx)
{
    int i = i_lim, j = j_lim;
    Medicao pivo = vetor[escolher_pivo(i_lim, j_lim, ctx)];
    int iguais = 0;

    for (int k = i_lim; k <= j_lim; k++) {
        int c = comparar(&vetor[k], &pivo, ctx);
        if (c < 0) {
            v_espelho[i] = vetor[k];
            i++;
        } else if (c > 0) {
            v_espelho[j] = vetor[k];
            j--;
        } else {
            vetor[i_lim + iguais] = vetor[k];  /* posição já lida */
            iguais++;
        }
    }
    for (int k = 0; k < iguais; k++) {
        v_espelho[i + k] = vetor[i_lim + k];  /* iguais == j - i + 1 */
    }
    for (int k = i_lim; k <= j_lim; k++) {
        vetor[k] = v_espelho[k];
    }
    *inicio_iguais = i;
    return j;
}

bool oe_particionamento(Medicao vetor[], int tam, int ordem,
                        Medicao *e_valor_para_ordem, Contexto *ctx)
{
    if ((ordem < 1) || (ordem > tam)) {
        return false;
    }
    /* O código-base criava o vetor espelho na pilha (VLA) a cada chamada;
       aqui ele é alocado uma vez, o que suporta N grande. */
    Medicao *v_espelho = malloc((size_t)tam * sizeof *v_espelho);
    if (v_espelho == NULL) {
        return false;
    }

    int i_lim = 0;
    int j_lim = tam - 1;
    bool achei = false;
    do {
        int inicio, fim;
        fim = particionamento(vetor, v_espelho, i_lim, j_lim, &inicio, ctx);
        /* O pivô (e suas cópias) ocupa as ordens inicio+1 .. fim+1. */
        if ((inicio + 1 <= ordem) && (ordem <= fim + 1)) {
            achei = true;
        } else if (ordem < inicio + 1) {
            j_lim = inicio - 1;  /* resposta está à esquerda do pivô */
        } else {
            i_lim = fim + 1;     /* resposta está à direita do pivô  */
        }
    } while (!achei);

    free(v_espelho);
    *e_valor_para_ordem = vetor[ordem - 1];
    return true;
}

/* ======================================================================== */

bool ordem_estatistica(Medicao vetor[], int tam, int ordem,
                       Medicao *e_valor_para_ordem, t_funcao chamadora,
                       Contexto *ctx)
{
    return chamadora(vetor, tam, ordem, e_valor_para_ordem, ctx);
}
