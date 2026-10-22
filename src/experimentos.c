#include "experimentos.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ordem_estatistica.h"

const int TAMANHOS_DESEMPENHO[QTD_TAMANHOS] = {1000, 5000, 10000};

#define QTD_ABORDAGENS 3

typedef struct {
    const char *nome;
    t_funcao    funcao;
} Abordagem;

static const Abordagem ABORDAGENS[QTD_ABORDAGENS] = {
    {"A (ordenação bolha)", oe_ordenacao},
    {"B (particionamento)", oe_particionamento},
    {"C (contagem)",        oe_contagem},
};

typedef struct {
    bool               ok;
    Medicao            valor;        /* registro encontrado            */
    unsigned long long comparacoes;
    double             ms;           /* tempo de CPU do algoritmo      */
} Resultado;

/* ------------------------------------------------------------------------ */
/* Utilidades                                                               */
/* ------------------------------------------------------------------------ */

int posicao_mediana(int n)     { return (n + 1) / 2; }
int posicao_percentil90(int n) { return (9 * n + 9) / 10; }

static void *alocar(size_t bytes)
{
    void *p = malloc(bytes ? bytes : 1);
    if (p == NULL) {
        fprintf(stderr, "Erro: memória insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

static double ms_desde(clock_t inicio)
{
    return 1000.0 * (double)(clock() - inicio) / CLOCKS_PER_SEC;
}

/* Inteiro com separador de milhar (49.995.000). Buffers rotativos permitem
   várias chamadas no mesmo printf. */
static const char *num(unsigned long long v)
{
    static char buffers[16][32];
    static int atual = 0;
    char *saida = buffers[atual];
    atual = (atual + 1) % 16;

    char tmp[32];
    int len = snprintf(tmp, sizeof tmp, "%llu", v);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0)
            saida[j++] = '.';
        saida[j++] = tmp[i];
    }
    saida[j] = '\0';
    return saida;
}

/* Completa `s` com espaços até `largura` caracteres VISÍVEIS (o "%-20s" do
   printf conta bytes e desalinha as tabelas com acentos em UTF-8). */
static const char *col(const char *s, int largura)
{
    static char buffers[8][128];
    static int atual = 0;
    char *saida = buffers[atual];
    atual = (atual + 1) % 8;

    int visiveis = 0;
    for (const char *p = s; *p; p++)
        if (((unsigned char)*p & 0xC0) != 0x80)
            visiveis++;
    int bytes = snprintf(saida, 128, "%s", s);
    while (visiveis < largura && bytes < 127) {
        saida[bytes++] = ' ';
        visiveis++;
    }
    saida[bytes] = '\0';
    return saida;
}

static void titulo(const char *texto)
{
    printf("\n================================================================"
           "================\n%s\n"
           "================================================================"
           "================\n", texto);
}

/* Copia os n primeiros registros (fora da medição) e executa a abordagem. */
static Resultado executar(int ab, const Medicao *dados, int n, int k,
                          Campo campo, Medicao *trabalho)
{
    Resultado r;
    Contexto ctx;
    contexto_iniciar(&ctx, campo);
    memcpy(trabalho, dados, (size_t)n * sizeof *trabalho);

    clock_t inicio = clock();
    r.ok = ordem_estatistica(trabalho, n, k, &r.valor, ABORDAGENS[ab].funcao,
                             &ctx);
    r.ms = ms_desde(inicio);
    r.comparacoes = ctx.comparacoes;
    return r;
}

/*
 * M menores valores de `campo`:
 *   A: ordena tudo e pega trabalho[0..m-1];
 *   B: seleciona o m-ésimo menor; o particionamento deixa os m menores em
 *      trabalho[0..m-1];
 *   C: contagem comparativa elemento a elemento.
 */
static Resultado listar_menores(int ab, const Medicao *dados, int n, int m,
                                Campo campo, Medicao *trabalho,
                                Medicao *lista, int *qtd)
{
    Resultado r;
    Contexto ctx;
    contexto_iniciar(&ctx, campo);
    memcpy(trabalho, dados, (size_t)n * sizeof *trabalho);
    if (m > n)
        m = n;

    clock_t inicio = clock();
    if (ab == 2) {
        *qtd = listar_menores_contagem(trabalho, n, m, lista, &ctx);
        r.ok = *qtd == m;
    } else {
        r.ok = ordem_estatistica(trabalho, n, m, &r.valor,
                                 ABORDAGENS[ab].funcao, &ctx);
        memcpy(lista, trabalho, (size_t)m * sizeof *lista);
        *qtd = m;
    }
    r.ms = ms_desde(inicio);
    r.comparacoes = ctx.comparacoes;
    return r;
}

/* Bolha usada só para EXIBIR uma lista curta em ordem; estas comparações
   não pertencem a nenhuma abordagem medida. */
static void ordenar_para_exibicao(Medicao *lista, int qtd, Campo campo)
{
    Contexto descartavel;
    contexto_iniciar(&descartavel, campo);
    bolha(lista, qtd, &descartavel);
}

/* ------------------------------------------------------------------------ */
/* Seção 2 — Consultas obrigatórias                                         */
/* ------------------------------------------------------------------------ */

static double consulta_ordem(const char *rotulo, const Medicao *dados, int n,
                             int k, Medicao *trabalho)
{
    printf("\n%s  (k = %s de N = %s)\n", rotulo, num((unsigned)k),
           num((unsigned)n));
    double valores[QTD_ABORDAGENS];
    for (int ab = 0; ab < QTD_ABORDAGENS; ab++) {
        Resultado r = executar(ab, dados, n, k, CAMPO_PM25, trabalho);
        valores[ab] = r.ok ? r.valor.nivel_pm25 : -1.0;
        printf("  Abordagem %s PM2.5 = %6.1f µg/m³   comparações = %13s"
               "   tempo = %9.3f ms\n",
               col(ABORDAGENS[ab].nome, 21), valores[ab], num(r.comparacoes), r.ms);
    }
    bool iguais = valores[0] == valores[1] && valores[1] == valores[2];
    printf("  -> %s\n", iguais ? "As três abordagens encontraram o mesmo valor."
                               : "ATENÇÃO: as abordagens divergiram!");
    return valores[0];
}

void executar_consultas(const Medicao *dados, int n, int m)
{
    titulo("2. CONSULTAS OBRIGATÓRIAS (conjunto completo)");
    Medicao *trabalho = alocar((size_t)n * sizeof *trabalho);

    consulta_ordem("Mediana exata do PM2.5", dados, n, posicao_mediana(n),
                   trabalho);

    double limiar = consulta_ordem("Limiar crítico do PM2.5 (percentil 90)",
                                   dados, n, posicao_percentil90(n), trabalho);
    int acima = 0;
    for (int i = 0; i < n; i++)
        if (dados[i].nivel_pm25 > limiar)
            acima++;
    printf("  Dias com PM2.5 acima de %.1f µg/m³ (alerta severo): %s de %s "
           "(%.1f%%)\n", limiar, num((unsigned)acima), num((unsigned)n),
           100.0 * acima / n);

    if (m > n)
        m = n;
    printf("\nOs %d dias mais secos (menores valores de umidade relativa)\n", m);
    Medicao *listas[QTD_ABORDAGENS];
    int qtds[QTD_ABORDAGENS];
    for (int ab = 0; ab < QTD_ABORDAGENS; ab++) {
        listas[ab] = alocar((size_t)m * sizeof(Medicao));
        Resultado r = listar_menores(ab, dados, n, m, CAMPO_UMIDADE, trabalho,
                                     listas[ab], &qtds[ab]);
        ordenar_para_exibicao(listas[ab], qtds[ab], CAMPO_UMIDADE);
        printf("  Abordagem %s %2d registros        comparações = %13s"
               "   tempo = %9.3f ms\n",
               col(ABORDAGENS[ab].nome, 21), qtds[ab], num(r.comparacoes), r.ms);
    }

    /* Com empates na fronteira, abordagens diferentes podem escolher dias
       diferentes com a MESMA umidade; o que deve coincidir são os valores. */
    bool iguais = qtds[0] == qtds[1] && qtds[1] == qtds[2];
    for (int i = 0; iguais && i < qtds[0]; i++)
        iguais = listas[0][i].umidade_porcentagem == listas[1][i].umidade_porcentagem &&
                 listas[1][i].umidade_porcentagem == listas[2][i].umidade_porcentagem;
    printf("  -> %s\n", iguais ? "As três abordagens encontraram os mesmos valores de umidade."
                               : "ATENÇÃO: as abordagens divergiram!");

    printf("\n   #  Data        Sensor  Umidade  Temperatura  PM2.5   (lista da Abordagem A)\n");
    for (int i = 0; i < qtds[0]; i++) {
        const Medicao *d = &listas[0][i];
        printf("  %2d  %s  %6d  %6d%%  %8.1f °C  %5.1f\n", i + 1, d->data,
               d->id_sensor, d->umidade_porcentagem, d->temperatura_c,
               d->nivel_pm25);
    }

    for (int ab = 0; ab < QTD_ABORDAGENS; ab++)
        free(listas[ab]);
    free(trabalho);
}

/* ------------------------------------------------------------------------ */
/* Seção 3 — Análise de desempenho                                          */
/* ------------------------------------------------------------------------ */

void analisar_desempenho(const Medicao *dados, int n, int m)
{
    titulo("3. ANÁLISE DE DESEMPENHO (número de comparações e tempo de CPU)");
    printf("Para cada N são usados os N primeiros registros do conjunto.\n"
           "Mediana e P90 sobre o PM2.5; dias mais secos (M = %d) sobre a umidade.\n\n", m);

    Medicao *trabalho = alocar((size_t)n * sizeof *trabalho);
    Medicao *lista = alocar((size_t)m * sizeof *lista);
    unsigned long long cmp_total[QTD_TAMANHOS][QTD_ABORDAGENS] = {{0}};
    double tempo_total[QTD_TAMANHOS][QTD_ABORDAGENS] = {{0}};

    printf("|     N  | Abordagem              |  Mediana (comp.) |      P90 (comp.) |"
           "  M secos (comp.) | Tempo total (ms) |\n");
    printf("|-------:|------------------------|-----------------:|-----------------:|"
           "-----------------:|-----------------:|\n");
    for (int t = 0; t < QTD_TAMANHOS; t++) {
        int tam = TAMANHOS_DESEMPENHO[t];
        if (tam > n) {
            printf("| %6d | (conjunto tem só %d registros) |\n", tam, n);
            continue;
        }
        for (int ab = 0; ab < QTD_ABORDAGENS; ab++) {
            Resultado med = executar(ab, dados, tam, posicao_mediana(tam),
                                     CAMPO_PM25, trabalho);
            Resultado p90 = executar(ab, dados, tam, posicao_percentil90(tam),
                                     CAMPO_PM25, trabalho);
            int qtd;
            Resultado sec = listar_menores(ab, dados, tam, m, CAMPO_UMIDADE,
                                           trabalho, lista, &qtd);
            cmp_total[t][ab] = med.comparacoes + p90.comparacoes + sec.comparacoes;
            tempo_total[t][ab] = med.ms + p90.ms + sec.ms;
            printf("| %6s | %s | %16s | %16s | %16s | %16.3f |\n",
                   num((unsigned)tam), col(ABORDAGENS[ab].nome, 22),
                   num(med.comparacoes), num(p90.comparacoes),
                   num(sec.comparacoes), tempo_total[t][ab]);
        }
    }

    printf("\nReferências teóricas:  N(N-1)/2 = bolha;  N² = pior caso da contagem\n");
    for (int t = 0; t < QTD_TAMANHOS; t++) {
        unsigned long long v = (unsigned long long)TAMANHOS_DESEMPENHO[t];
        printf("  N = %6s:  N(N-1)/2 = %13s   N² = %13s\n", num(v),
               num(v * (v - 1) / 2), num(v * v));
    }

    if (n >= TAMANHOS_DESEMPENHO[QTD_TAMANHOS - 1]) {
        printf("\nCrescimento de N = 1.000 para N = 10.000 (N 10x maior) — Questão 4,"
               " somando as três consultas:\n");
        for (int ab = 0; ab < QTD_ABORDAGENS; ab++) {
            double fc = (double)cmp_total[2][ab] / (double)cmp_total[0][ab];
            double ft = tempo_total[0][ab] > 0
                            ? tempo_total[2][ab] / tempo_total[0][ab] : 0.0;
            printf("  Abordagem %s comparações x%7.1f   tempo x%7.1f\n",
                   col(ABORDAGENS[ab].nome, 21), fc, ft);
        }
        printf("  (crescimento linear ~ x10; quadrático ~ x100)\n");
    }
    free(lista);
    free(trabalho);
}

/* ------------------------------------------------------------------------ */
/* Seção 4 — Melhor e pior caso                                             */
/* ------------------------------------------------------------------------ */

typedef enum {
    CEN_ALEATORIO,
    CEN_CRESCENTE,
    CEN_DECRESCENTE,
    CEN_TODOS_IGUAIS,
    CEN_RESPOSTA_PRIMEIRA,
    CEN_RESPOSTA_ULTIMA,
    CEN_ADVERSARIO_PIVO,
    QTD_CENARIOS
} Cenario;

static const char *const NOME_CENARIO[QTD_CENARIOS] = {
    "aleatório, valores distintos",
    "já ordenado (crescente)",
    "ordem decrescente",
    "todos os valores iguais",
    "resposta na 1ª posição",
    "resposta na última posição",
    "adversário do pivô central",
};

/*
 * Gera chaves 1..n que levam o particionamento com pivô central ao pior
 * caso para a ordem k: em toda etapa o elemento do meio é o MENOR do
 * subvetor, então cada partição descarta só o próprio pivô. Para isso o
 * algoritmo é simulado sobre as posições originais: o elemento que cai no
 * meio recebe o menor valor ainda não usado, e os demais (todos "maiores")
 * são movidos exatamente como o vetor espelho os moveria.
 */
static void gerar_adversario_pivo_central(float *chave, int n, int k)
{
    int *pos = alocar((size_t)n * sizeof *pos);
    int *espelho = alocar((size_t)n * sizeof *espelho);
    for (int i = 0; i < n; i++)
        pos[i] = i;

    int proximo = 1, i_lim = 0, j_lim = n - 1;
    while (i_lim <= k - 1) {
        int meio = (i_lim + j_lim) / 2;
        chave[pos[meio]] = (float)proximo++;
        int j = j_lim;
        for (int x = i_lim; x <= j_lim; x++)
            if (x != meio)
                espelho[j--] = pos[x];
        espelho[i_lim] = pos[meio];
        for (int x = i_lim; x <= j_lim; x++)
            pos[x] = espelho[x];
        i_lim++;
    }
    for (int x = i_lim; x <= j_lim; x++)
        chave[pos[x]] = (float)proximo++;
    free(espelho);
    free(pos);
}

static void montar_cenario(Medicao *v, int n, int k, Cenario cen, Rng *rng)
{
    float *chave = alocar((size_t)n * sizeof *chave);
    for (int i = 0; i < n; i++)
        chave[i] = (float)(i + 1);

    if (cen == CEN_ALEATORIO || cen == CEN_RESPOSTA_PRIMEIRA ||
        cen == CEN_RESPOSTA_ULTIMA) {
        for (int i = n - 1; i > 0; i--) {  /* embaralhamento de Fisher-Yates */
            int j = (int)rng_intervalo(rng, (size_t)i + 1);
            float t = chave[i]; chave[i] = chave[j]; chave[j] = t;
        }
        int alvo = cen == CEN_RESPOSTA_PRIMEIRA ? 0 : n - 1;
        if (cen != CEN_ALEATORIO)
            for (int i = 0; i < n; i++)
                if (chave[i] == (float)k) {
                    float t = chave[i]; chave[i] = chave[alvo]; chave[alvo] = t;
                    break;
                }
    } else if (cen == CEN_DECRESCENTE) {
        for (int i = 0; i < n; i++)
            chave[i] = (float)(n - i);
    } else if (cen == CEN_TODOS_IGUAIS) {
        for (int i = 0; i < n; i++)
            chave[i] = 50.0f;
    } else if (cen == CEN_ADVERSARIO_PIVO) {
        gerar_adversario_pivo_central(chave, n, k);
    }

    for (int i = 0; i < n; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].id_sensor = i;
        strcpy(v[i].data, "2026-09-01");
        v[i].nivel_pm25 = chave[i];
    }
    free(chave);
}

void analisar_casos(int n, uint64_t semente)
{
    char texto[96];
    snprintf(texto, sizeof texto,
             "4. MELHOR E PIOR CASO MEDIDOS (N = %d, k = mediana)", n);
    titulo(texto);

    int k = posicao_mediana(n);
    Medicao *dados = alocar((size_t)n * sizeof *dados);
    Medicao *trabalho = alocar((size_t)n * sizeof *trabalho);
    Rng rng;
    rng_semear(&rng, semente);

    printf("| Cenário                        | A (bolha)       | B (partição)    | C (contagem)    |\n");
    printf("|--------------------------------|----------------:|----------------:|----------------:|\n");
    for (int c = 0; c < QTD_CENARIOS; c++) {
        montar_cenario(dados, n, k, (Cenario)c, &rng);
        unsigned long long cmp[QTD_ABORDAGENS];
        for (int ab = 0; ab < QTD_ABORDAGENS; ab++)
            cmp[ab] = executar(ab, dados, n, k, CAMPO_PM25, trabalho).comparacoes;
        printf("| %s | %15s | %15s | %15s |\n", col(NOME_CENARIO[c], 30),
               num(cmp[0]), num(cmp[1]), num(cmp[2]));
    }

    unsigned long long N = (unsigned long long)n, K = (unsigned long long)k;
    printf("\nFórmulas para N = %s, k = %s:\n", num(N), num(K));
    printf("  A: sempre N(N-1)/2 = %s (a bolha do código-base não para antes)\n",
           num(N * (N - 1) / 2));
    printf("  B: melhor caso N = %s (1ª partição acerta);"
           " pior caso sum_{t=0}^{k-1}(N-t) = %s\n",
           num(N), num(K * N - K * (K - 1) / 2));
    printf("  C: melhor caso N = %s (resposta no 1º elemento);"
           " pior caso N² = %s (resposta no último)\n", num(N), num(N * N));

    free(trabalho);
    free(dados);
}

/* ------------------------------------------------------------------------ */
/* Seção 5 — Questões de acompanhamento prático                             */
/* ------------------------------------------------------------------------ */

static void preencher_pm25(Medicao *v, const int *valores, int n)
{
    for (int i = 0; i < n; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].id_sensor = i + 1;
        v[i].nivel_pm25 = (float)valores[i];
    }
}

static void imprimir_lista(const int *valores, int n)
{
    printf("[");
    for (int i = 0; i < n; i++)
        printf("%s%d", i ? ", " : "", valores[i]);
    printf("]");
}

void questao1_rastreio_contagem(void)
{
    titulo("5.1 QUESTÃO 1 — Rastreando a Abordagem C (contagem comparativa)");
    const int valores[] = {45, 90, 30, 90, 60};
    const int n = 5, k = 3;
    Medicao v[5], resposta;
    preencher_pm25(v, valores, n);

    printf("Lista: ");
    imprimir_lista(valores, n);
    printf(", procurando a mediana (k = %d).\n"
           "Cada elemento é comparado com TODOS os %d elementos (inclusive ele mesmo,\n"
           "como no código-base). Ele ocupa as posições (menores+1)..(menores ou iguais).\n\n",
           k, n);

    Contexto ctx;
    contexto_iniciar(&ctx, CAMPO_PM25);
    ctx.rastreio = stdout;
    bool ok = ordem_estatistica(v, n, k, &resposta, oe_contagem, &ctx);
    printf("\nResultado: %d-ª ordem estatística = %g\n", k,
           ok ? resposta.nivel_pm25 : -1.0);
    printf("Total de comparações diretas: %llu (5 elementos avaliados x 5 comparações)\n",
           ctx.comparacoes);
}

/*
 * Versão do código-base, que conta só os ESTRITAMENTE menores e aceita o
 * elemento quando menores + 1 == ordem. Aqui o laço ganhou um limite
 * (i < tam) apenas para podermos mostrar a falha sem ler fora do vetor:
 * no código-base o do-while continuaria em vetor[5], vetor[6], ...
 */
static bool oe_contagem_somente_menores(Medicao vetor[], int tam, int ordem,
                                        Medicao *e_valor, Contexto *ctx)
{
    int i = 0, ordem_valor;
    do {
        int menores = 0;
        for (int j = 0; j < tam; j++)
            if (comparar(&vetor[j], &vetor[i], ctx) < 0)
                menores++;
        ordem_valor = menores + 1;
        i++;
    } while (ordem_valor != ordem && i < tam);
    if (ordem_valor != ordem)
        return false;
    *e_valor = vetor[i - 1];
    return true;
}

void questao2_empates(void)
{
    titulo("5.2 QUESTÃO 2 — Medições iguais (empates) na Abordagem C");
    const int valores[] = {45, 90, 30, 90, 90};
    const int n = 5;
    Medicao v[5];
    preencher_pm25(v, valores, n);

    printf("Lista com três dias de pico idêntico: ");
    imprimir_lista(valores, n);
    printf("  (ordenada: [30, 45, 90, 90, 90])\n\n");
    printf("  k | só estritamente menores (<)           | menores e menores ou iguais (<, <=)\n");
    printf("  --|----------------------------------------|------------------------------------\n");
    for (int k = 1; k <= n; k++) {
        Medicao r1, r2;
        Contexto c1, c2;
        contexto_iniciar(&c1, CAMPO_PM25);
        contexto_iniciar(&c2, CAMPO_PM25);
        bool ok1 = oe_contagem_somente_menores(v, n, k, &r1, &c1);
        bool ok2 = oe_contagem(v, n, k, &r2, &c2);
        char s1[64], s2[64];
        if (ok1)
            snprintf(s1, sizeof s1, "%g (%llu comparações)", r1.nivel_pm25, c1.comparacoes);
        else
            snprintf(s1, sizeof s1, "NÃO ENCONTRA: passaria do fim do vetor");
        snprintf(s2, sizeof s2, "%g (%llu comparações)", ok2 ? r2.nivel_pm25 : -1.0,
                 c2.comparacoes);
        printf("  %d | %s | %s\n", k, col(s1, 38), s2);
    }
    printf("\nCom '<' cada 90 tem 2 menores e recebe a ordem 3; as ordens 4 e 5 não\n"
           "pertencem a ninguém e o do-while do código-base sai do vetor. A correção\n"
           "conta também os '<=' (3 e 5) e aceita o valor se menores < k <= menores_ou_iguais.\n");
}

void questao3_escolha_do_pivo(const Medicao *dados, int n, uint64_t semente)
{
    titulo("5.3 QUESTÃO 3 — Impacto da escolha do pivô (Abordagem B)");
    Medicao *ordenado = alocar((size_t)n * sizeof *ordenado);
    Medicao *trabalho = alocar((size_t)n * sizeof *trabalho);
    memcpy(ordenado, dados, (size_t)n * sizeof *ordenado);
    Contexto ctx;
    contexto_iniciar(&ctx, CAMPO_PM25);
    bolha(ordenado, n, &ctx);  /* entrada "do dia menos poluído ao mais poluído" */

    const char *nomes[3] = {"primeiro elemento", "aleatório", "central (exigido)"};
    const EstrategiaPivo estrategias[3] = {PIVO_PRIMEIRO, PIVO_ALEATORIO, PIVO_CENTRAL};
    const Medicao *entradas[2] = {dados, ordenado};
    const char *nomes_entrada[2] = {"ordem original", "já ordenada"};

    printf("N = %d registros de PM2.5 (com valores repetidos).\n\n", n);
    printf("| Entrada         | Pivô               | Mediana (comp.) | P90 (comp.)     | Tempo (ms) |\n");
    printf("|-----------------|--------------------|----------------:|----------------:|-----------:|\n");
    for (int e = 0; e < 2; e++) {
        for (int p = 0; p < 3; p++) {
            Rng rng;
            rng_semear(&rng, semente);
            unsigned long long cmp[2];
            double ms = 0;
            int ks[2] = {posicao_mediana(n), posicao_percentil90(n)};
            for (int q = 0; q < 2; q++) {
                Medicao r;
                contexto_iniciar(&ctx, CAMPO_PM25);
                ctx.pivo = estrategias[p];
                ctx.rng = &rng;
                memcpy(trabalho, entradas[e], (size_t)n * sizeof *trabalho);
                clock_t inicio = clock();
                oe_particionamento(trabalho, n, ks[q], &r, &ctx);
                ms += ms_desde(inicio);
                cmp[q] = ctx.comparacoes;
            }
            printf("| %s | %s | %15s | %15s | %10.3f |\n", col(nomes_entrada[e], 15),
                   col(nomes[p], 18), num(cmp[0]), num(cmp[1]), ms);
        }
    }
    printf("\nCom a entrada ordenada e pivô = primeiro elemento, cada partição só\n"
           "descarta o próprio pivô (e suas cópias): o custo vira quadrático, como a bolha.\n");
    free(trabalho);
    free(ordenado);
}

void questao5_reutilizacao(const Medicao *dados, int n)
{
    titulo("5.5 QUESTÃO 5 — Reutilização (A) vs. velocidade única (B)");
    Medicao *trabalho = alocar((size_t)n * sizeof *trabalho);
    int k_med = posicao_mediana(n), k_p90 = posicao_percentil90(n);
    Medicao r;
    Contexto ctx;

    /* A: ordena uma vez; a segunda consulta é acesso direto. */
    memcpy(trabalho, dados, (size_t)n * sizeof *trabalho);
    contexto_iniciar(&ctx, CAMPO_PM25);
    oe_ordenacao(trabalho, n, k_med, &r, &ctx);
    unsigned long long a1 = ctx.comparacoes;
    double a_med = r.nivel_pm25, a_p90 = trabalho[k_p90 - 1].nivel_pm25;

    /* B: cada consulta é um novo particionamento (sobre o mesmo vetor, já
       reorganizado pela primeira, e sobre uma cópia nova, para comparar). */
    memcpy(trabalho, dados, (size_t)n * sizeof *trabalho);
    contexto_iniciar(&ctx, CAMPO_PM25);
    oe_particionamento(trabalho, n, k_med, &r, &ctx);
    unsigned long long b1 = ctx.comparacoes;
    contexto_iniciar(&ctx, CAMPO_PM25);
    oe_particionamento(trabalho, n, k_p90, &r, &ctx);
    unsigned long long b2 = ctx.comparacoes;
    double b_p90 = r.nivel_pm25;
    memcpy(trabalho, dados, (size_t)n * sizeof *trabalho);
    contexto_iniciar(&ctx, CAMPO_PM25);
    oe_particionamento(trabalho, n, k_p90, &r, &ctx);
    unsigned long long b2_novo = ctx.comparacoes;

    printf("N = %d. Primeiro a mediana (%.1f µg/m³), depois o P90 (%.1f µg/m³).\n\n",
           n, a_med, a_p90);
    printf("| Estratégia                           | Mediana       | P90           | Total         |\n");
    printf("|--------------------------------------|--------------:|--------------:|--------------:|\n");
    printf("| A: ordena 1 vez + acesso direto      | %13s | %13s | %13s |\n",
           num(a1), num(0), num(a1));
    printf("| B: novo particionamento (mesmo vetor)| %13s | %13s | %13s |\n",
           num(b1), num(b2), num(b1 + b2));
    printf("| B: P90 sobre uma cópia nova          | %13s | %13s | %13s |\n",
           "-", num(b2_novo), "-");
    printf("\nA: depois de ordenado, qualquer outra ordem estatística sai de vetor[k-1]\n"
           "sem nenhuma comparação. B: o vetor só fica parcialmente organizado, então\n"
           "cada pergunta exige um novo particionamento (P90 = %.1f nos dois casos).\n",
           b_p90);
    free(trabalho);
}
