#include "medicao.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA   512
#define NUM_CAMPOS  5

double medicao_chave(const Medicao *m, Campo campo)
{
    switch (campo) {
    case CAMPO_TEMPERATURA: return m->temperatura_c;
    case CAMPO_UMIDADE:     return m->umidade_porcentagem;
    case CAMPO_PM25:        return m->nivel_pm25;
    }
    return 0.0;
}

const char *campo_nome(Campo campo)
{
    switch (campo) {
    case CAMPO_TEMPERATURA: return "Temperatura_C";
    case CAMPO_UMIDADE:     return "Umidade_Porcentagem";
    case CAMPO_PM25:        return "Nivel_PM25";
    }
    return "?";
}

/* ------------------------------------------------------------------------ */
/* Vetor dinâmico                                                           */
/* ------------------------------------------------------------------------ */

void conjunto_iniciar(Conjunto *c)
{
    c->itens = NULL;
    c->qtd = 0;
    c->capacidade = 0;
}

int conjunto_reservar(Conjunto *c, size_t capacidade)
{
    if (capacidade <= c->capacidade)
        return 0;
    Medicao *novo = realloc(c->itens, capacidade * sizeof *novo);
    if (novo == NULL)
        return -1;
    c->itens = novo;
    c->capacidade = capacidade;
    return 0;
}

int conjunto_adicionar(Conjunto *c, const Medicao *m)
{
    if (c->qtd == c->capacidade &&
        conjunto_reservar(c, c->capacidade ? 2 * c->capacidade : 1024) != 0)
        return -1;
    c->itens[c->qtd++] = *m;
    return 0;
}

void conjunto_liberar(Conjunto *c)
{
    free(c->itens);
    conjunto_iniciar(c);
}

/* ------------------------------------------------------------------------ */
/* Leitura e gravação de CSV                                                */
/* ------------------------------------------------------------------------ */

/* Remove espaços (incluindo '\r' de arquivos CRLF) do início e do fim. */
static char *aparar(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *fim = s + strlen(s);
    while (fim > s && isspace((unsigned char)fim[-1]))
        *--fim = '\0';
    return s;
}

static int ler_inteiro(const char *s, int *saida)
{
    char *fim;
    errno = 0;
    long v = strtol(s, &fim, 10);
    if (*s == '\0' || *fim != '\0' || errno != 0)
        return -1;
    *saida = (int)v;
    return 0;
}

static int ler_real(const char *s, float *saida)
{
    char *fim;
    errno = 0;
    double v = strtod(s, &fim);
    if (*s == '\0' || *fim != '\0' || errno != 0 || !isfinite(v))
        return -1;
    *saida = (float)v;
    return 0;
}

/* Aceita apenas datas no formato AAAA-MM-DD. */
static int data_valida(const char *s)
{
    if (strlen(s) != TAM_DATA - 1)
        return 0;
    for (int i = 0; i < TAM_DATA - 1; i++) {
        if (i == 4 || i == 7) {
            if (s[i] != '-')
                return 0;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

/* Divide a linha em exatamente NUM_CAMPOS campos separados por vírgula. */
static int separar_campos(char *linha, char *campos[NUM_CAMPOS])
{
    int n = 0;
    char *p = linha;
    for (;;) {
        if (n == NUM_CAMPOS)
            return -1;
        campos[n++] = p;
        char *virgula = strchr(p, ',');
        if (virgula == NULL)
            break;
        *virgula = '\0';
        p = virgula + 1;
    }
    for (int i = 0; i < n; i++)
        campos[i] = aparar(campos[i]);
    return n == NUM_CAMPOS ? 0 : -1;
}

static int interpretar_linha(char *linha, Medicao *m)
{
    char *campos[NUM_CAMPOS];
    if (separar_campos(linha, campos) != 0)
        return -1;
    if (ler_inteiro(campos[0], &m->id_sensor) != 0 ||
        !data_valida(campos[1]) ||
        ler_real(campos[2], &m->temperatura_c) != 0 ||
        ler_inteiro(campos[3], &m->umidade_porcentagem) != 0 ||
        ler_real(campos[4], &m->nivel_pm25) != 0)
        return -1;
    if (m->umidade_porcentagem < 0 || m->umidade_porcentagem > 100 ||
        m->nivel_pm25 < 0.0f)
        return -1;
    memcpy(m->data, campos[1], TAM_DATA);
    return 0;
}

int ler_csv(const char *caminho, Conjunto *c, size_t *invalidas)
{
    FILE *arq = fopen(caminho, "r");
    if (arq == NULL)
        return -1;

    char linha[TAM_LINHA];
    int primeira = 1;
    *invalidas = 0;

    while (fgets(linha, sizeof linha, arq) != NULL) {
        /* Linha maior que o buffer: descarta o restante e a considera inválida. */
        if (strchr(linha, '\n') == NULL && !feof(arq)) {
            int ch;
            while ((ch = fgetc(arq)) != '\n' && ch != EOF)
                ;
            (*invalidas)++;
            primeira = 0;
            continue;
        }

        char *conteudo = aparar(linha);
        if (*conteudo == '\0')
            continue;

        /* Cabeçalho: primeira linha não vazia que não começa com número. */
        if (primeira) {
            primeira = 0;
            if (!isdigit((unsigned char)conteudo[0]) && conteudo[0] != '-')
                continue;
        }

        Medicao m;
        if (interpretar_linha(conteudo, &m) != 0) {
            (*invalidas)++;
            continue;
        }
        if (conjunto_adicionar(c, &m) != 0) {
            fclose(arq);
            return -1;
        }
    }
    fclose(arq);
    return 0;
}

int gravar_csv(const char *caminho, const Medicao *v, size_t n)
{
    FILE *arq = fopen(caminho, "w");
    if (arq == NULL)
        return -1;
    fprintf(arq, "ID_Sensor,Data,Temperatura_C,Umidade_Porcentagem,Nivel_PM25\n");
    for (size_t i = 0; i < n; i++)
        fprintf(arq, "%d,%s,%.1f,%d,%.1f\n", v[i].id_sensor, v[i].data,
                v[i].temperatura_c, v[i].umidade_porcentagem, v[i].nivel_pm25);
    return fclose(arq) == 0 ? 0 : -1;
}

/* ------------------------------------------------------------------------ */
/* Simulação                                                                */
/* ------------------------------------------------------------------------ */

#define DIAS_PERIODO 92  /* 01/08 a 31/10: 31 + 30 + 31 dias */

static void data_do_dia(int dia, char saida[TAM_DATA])
{
    int mes, dia_mes;
    if (dia < 31) {
        mes = 8;
        dia_mes = dia + 1;
    } else if (dia < 61) {
        mes = 9;
        dia_mes = dia - 30;
    } else {
        mes = 10;
        dia_mes = dia - 60;
    }
    char tmp[32];  /* folga para o compilador não acusar truncamento */
    snprintf(tmp, sizeof tmp, "2026-%02d-%02d", mes, dia_mes);
    memcpy(saida, tmp, TAM_DATA);
}

static double arredondar_1casa(double x)
{
    return floor(x * 10.0 + 0.5) / 10.0;
}

static double limitar(double x, double minimo, double maximo)
{
    return x < minimo ? minimo : (x > maximo ? maximo : x);
}

/*
 * Normal N(media, desvio) truncada em [minimo, maximo]: sorteia de novo
 * quando o valor cai fora da faixa. Só "corta" no limite como último
 * recurso, evitando acumular centenas de registros exatamente no limite.
 */
static double normal_truncada(Rng *rng, double media, double desvio,
                              double minimo, double maximo)
{
    for (int tentativa = 0; tentativa < 1000; tentativa++) {
        double x = media + desvio * rng_normal(rng);
        if (x >= minimo && x <= maximo)
            return x;
    }
    return limitar(media, minimo, maximo);
}

void simular_medicoes(Medicao *destino, size_t qtd, Rng *rng)
{
    /*
     * Intensidade de fumaça de cada dia: uma curva que cresce até o fim de
     * setembro (auge das queimadas) multiplicada por episódios aleatórios de
     * "fumaceiro" que podem até triplicar a poluição do dia.
     */
    double fumaca[DIAS_PERIODO];
    for (int d = 0; d < DIAS_PERIODO; d++) {
        double z = (d - 58) / 18.0;
        double u = rng_uniforme(rng);
        fumaca[d] = (1.0 + 2.2 * exp(-z * z)) * (1.0 + 2.0 * u * u * u * u);
    }

    for (size_t i = 0; i < qtd; i++) {
        Medicao *m = &destino[i];
        int dia = (int)rng_intervalo(rng, DIAS_PERIODO);
        double f = fumaca[dia];

        double temp = normal_truncada(rng, 30.5 + 1.2 * (f - 1.0), 2.0,
                                      24.0, 40.0);
        temp = arredondar_1casa(temp);

        double umid = normal_truncada(rng, 72.0 - 3.2 * (temp - 30.5)
                                               - 5.0 * (f - 1.0),
                                      7.0, 12.0, 98.0);
        umid = floor(umid + 0.5);

        /* Log-normal: 16·f·e^(0,45·z), com z truncado para PM2.5 em [3, 450]. */
        double z = normal_truncada(rng, 0.0, 1.0, log(3.0 / (16.0 * f)) / 0.45,
                                   log(450.0 / (16.0 * f)) / 0.45);
        double pm25 = arredondar_1casa(limitar(16.0 * f * exp(0.45 * z), 3.0, 450.0));

        m->id_sensor = 100 + (int)rng_intervalo(rng, 100);
        data_do_dia(dia, m->data);
        m->temperatura_c = (float)temp;
        m->umidade_porcentagem = (int)umid;
        m->nivel_pm25 = (float)pm25;
    }
}
