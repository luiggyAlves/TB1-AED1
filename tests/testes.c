/*
 * Testes das três abordagens contra uma referência por força bruta, com
 * muitos valores repetidos (o caso que quebrava o código-base).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aleatorio.h"
#include "medicao.h"
#include "ordem_estatistica.h"

static int falhas = 0, verificacoes = 0;

#define VERIFICA(cond, ...)                                         \
    do {                                                            \
        verificacoes++;                                             \
        if (!(cond)) {                                              \
            falhas++;                                               \
            if (falhas <= 20) {                                     \
                printf("FALHA (%s:%d): ", __FILE__, __LINE__);      \
                printf(__VA_ARGS__);                                \
                printf("\n");                                       \
            }                                                       \
        }                                                           \
    } while (0)

/* k-ésimo menor por definição: valor x tal que #(<x) < k <= #(<=x). */
static float referencia(const Medicao *v, int n, int k)
{
    for (int i = 0; i < n; i++) {
        int menores = 0, menores_ou_iguais = 0;
        for (int j = 0; j < n; j++) {
            menores += v[j].nivel_pm25 < v[i].nivel_pm25;
            menores_ou_iguais += v[j].nivel_pm25 <= v[i].nivel_pm25;
        }
        if (menores < k && k <= menores_ou_iguais)
            return v[i].nivel_pm25;
    }
    return -1.0f;
}

/* Mesmo multiconjunto de registros (ids distintos 0..n-1). */
static int mesma_permutacao(const Medicao *a, int n)
{
    char *visto = calloc((size_t)n, 1);
    int ok = 1;
    for (int i = 0; i < n && ok; i++) {
        int id = a[i].id_sensor;
        ok = id >= 0 && id < n && !visto[id];
        if (ok)
            visto[id] = 1;
    }
    free(visto);
    return ok;
}

static void gerar(Medicao *v, int n, int faixa, Rng *rng)
{
    for (int i = 0; i < n; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].id_sensor = i;
        v[i].nivel_pm25 = (float)rng_intervalo(rng, (size_t)faixa);
        v[i].umidade_porcentagem = (int)rng_intervalo(rng, (size_t)faixa);
    }
}

static void testar_selecao(Rng *rng)
{
    const t_funcao funcoes[3] = {oe_ordenacao, oe_particionamento, oe_contagem};
    const char *nomes[3] = {"A", "B", "C"};
    Medicao v[80], w[80], r;

    for (int rodada = 0; rodada < 400; rodada++) {
        int n = 1 + (int)rng_intervalo(rng, 70);
        int faixa = 1 + (int)rng_intervalo(rng, rodada % 2 ? 5 : 1000);
        gerar(v, n, faixa, rng);

        for (int k = 1; k <= n; k++) {
            float esperado = referencia(v, n, k);
            for (int ab = 0; ab < 3; ab++) {
                for (int p = 0; p < (ab == 1 ? 3 : 1); p++) {
                    Contexto ctx;
                    contexto_iniciar(&ctx, CAMPO_PM25);
                    ctx.pivo = (EstrategiaPivo)p;
                    ctx.rng = rng;
                    memcpy(w, v, sizeof(Medicao) * (size_t)n);
                    int ok = ordem_estatistica(w, n, k, &r, funcoes[ab], &ctx);
                    VERIFICA(ok && r.nivel_pm25 == esperado,
                             "%s pivo=%d n=%d k=%d: obteve %g, esperado %g",
                             nomes[ab], p, n, k, r.nivel_pm25, esperado);
                    VERIFICA(mesma_permutacao(w, n),
                             "%s n=%d k=%d: registros perdidos/duplicados", nomes[ab], n, k);
                    if (ab == 1) {
                        /* B deixa os k menores em w[0..k-1]. */
                        for (int i = 0; i < n; i++)
                            VERIFICA(i < k ? w[i].nivel_pm25 <= esperado
                                           : w[i].nivel_pm25 >= esperado,
                                     "B n=%d k=%d: posição %d fora da partição", n, k, i);
                    }
                    if (ab == 0) {
                        for (int i = 0; i + 1 < n; i++)
                            VERIFICA(w[i].nivel_pm25 <= w[i + 1].nivel_pm25,
                                     "A n=%d: vetor não ordenado", n);
                        VERIFICA(ctx.comparacoes == (unsigned long long)n * (n - 1) / 2,
                                 "A n=%d: %llu comparações", n, ctx.comparacoes);
                    }
                }
            }
        }
        /* Ordens inválidas são recusadas. */
        for (int ab = 0; ab < 3; ab++) {
            Contexto ctx;
            contexto_iniciar(&ctx, CAMPO_PM25);
            VERIFICA(!funcoes[ab](v, n, 0, &r, &ctx) && !funcoes[ab](v, n, n + 1, &r, &ctx),
                     "%s aceitou ordem inválida", nomes[ab]);
        }
    }
}

static void testar_listagem(Rng *rng)
{
    Medicao v[80], saida[80];
    for (int rodada = 0; rodada < 300; rodada++) {
        int n = 1 + (int)rng_intervalo(rng, 70);
        gerar(v, n, 1 + (int)rng_intervalo(rng, rodada % 2 ? 4 : 100), rng);
        for (int m = 1; m <= n + 2; m++) {
            Contexto ctx;
            contexto_iniciar(&ctx, CAMPO_PM25);
            int qtd = listar_menores_contagem(v, n, m, saida, &ctx);
            int esperado_qtd = m < n ? m : n;
            VERIFICA(qtd == esperado_qtd, "listagem n=%d m=%d: %d itens", n, m, qtd);

            /* Ordenar a saída e comparar com os m primeiros valores ordenados. */
            Medicao ordenado[80];
            memcpy(ordenado, v, sizeof(Medicao) * (size_t)n);
            Contexto aux;
            contexto_iniciar(&aux, CAMPO_PM25);
            bolha(ordenado, n, &aux);
            bolha(saida, qtd, &aux);
            for (int i = 0; i < qtd; i++)
                VERIFICA(saida[i].nivel_pm25 == ordenado[i].nivel_pm25,
                         "listagem n=%d m=%d: posição %d", n, m, i);
            for (int i = 0; i < qtd; i++)
                for (int j = i + 1; j < qtd; j++)
                    VERIFICA(saida[i].id_sensor != saida[j].id_sensor,
                             "listagem n=%d m=%d: registro repetido", n, m);
        }
    }
}

static void testar_questao1(void)
{
    const float valores[] = {45, 90, 30, 90, 60};
    Medicao v[5], r;
    for (int i = 0; i < 5; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].nivel_pm25 = valores[i];
    }
    Contexto ctx;
    contexto_iniciar(&ctx, CAMPO_PM25);
    VERIFICA(oe_contagem(v, 5, 3, &r, &ctx) && r.nivel_pm25 == 60.0f,
             "Questão 1: mediana %g", r.nivel_pm25);
    VERIFICA(ctx.comparacoes == 25, "Questão 1: %llu comparações", ctx.comparacoes);
}

int main(void)
{
    Rng rng;
    rng_semear(&rng, 12345);
    testar_selecao(&rng);
    testar_listagem(&rng);
    testar_questao1();
    printf("%d verificações, %d falhas\n", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
