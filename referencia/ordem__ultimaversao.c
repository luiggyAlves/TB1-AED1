#include "stdio.h"
#include "stdbool.h"

#define TAM 6
int opcao;

static void trocar(int vetor[], int i, int k){
    int elem = vetor[i];
    vetor[i] = vetor[k];
    vetor[k] = elem;
}

void bolha(int vetor[], int tam){
    for(int j=0;j<tam-1;j++){
        for (int i=0;i<tam-1-j;i++)
        {
            if(vetor[i]>vetor[i+1]){
                trocar(vetor,i,i+1);
            }
        }
        
    }
}

static int contagem(int vetor[], int tam, int valor){
    int conta=0;
    for(int i=0;i<tam;i++){
        if (vetor[i] < valor){
            conta++;
        }
    }
    return conta;
}


bool oe_ordenacao(int vetor[], int tam, int ordem, int* e_valor_para_ordem){
    if ((ordem < 1) || (ordem>tam)){
        return false;
    }
    bolha(vetor, tam);
    
    *e_valor_para_ordem = vetor[ordem-1];
    return true;
}

bool oe_contagem(int vetor[], int tam, int ordem, int*e_valor_para_ordem){
    if ((ordem < 1) || (ordem>tam)){
        return false;
    }
    int i=0, valor, ordem_valor;
    do{
        valor = vetor[i];
        ordem_valor = contagem(vetor, tam, valor) + 1;
        i++;
    }while(ordem_valor != ordem);
    *e_valor_para_ordem = valor;
      return true;    
}

static int particionamento(int vetor[], int i_lim, int j_lim){
    int i = i_lim, j=j_lim;
    int i_pivo = (i+j)/2;
    int v_espelho[j+1];
    for(int k=i_lim;k<=j_lim;k++){
        if (vetor[k] < vetor[i_pivo]){
            v_espelho[i] = vetor[k];
            i++;
        }else if(vetor[k]>vetor[i_pivo]){
            v_espelho[j] = vetor[k];
            j--;
        }
    }
    v_espelho[j]=vetor[i_pivo];
    for(int k=i_lim;k<=j_lim;k++){
        vetor[k] = v_espelho[k];
    }
    return j;
}

static int particionamento_semvetoraux(int vetor[], int i_lim, int j_lim){
    int i = i_lim, j=j_lim;
    int i_pivo = (i+j)/2;
    int pivo = vetor[i_pivo];
    do{
        while((vetor[i] < pivo) && (i<=j_lim)){
            i++;
        }
        while((vetor[j]>pivo) && (j>=0)){
            j--;
        }
        if (i<j){
            trocar(vetor, i, j);
        }
    }while(i<j);

    return j;
}

bool oe_particionamento(int vetor[], int tam, int ordem, int*e_valor_para_ordem){
    if ((ordem < 1) || (ordem>tam)){
        return false;
    }
    int ordem_pivo;
    int i_lim = 0;
    int j_lim = tam-1;
    bool achei = false;
    do{
        ordem_pivo = particionamento(vetor, i_lim, j_lim) + 1;
        if (ordem_pivo == ordem){
            achei = true;
        }else{
            if (ordem_pivo > ordem){
                j_lim = ordem_pivo - 1;
            }else{
                i_lim = ordem_pivo;
            }
        }
    }while(!achei);
    
    *e_valor_para_ordem = vetor[ordem_pivo-1];
    return true;
}

typedef bool(*t_funcao)(int vetor[], int tam, int ordem, int* e_valor);

bool ordem_estatistica(int vetor[], int tam, int ordem, int*e_valor_para_ordem, t_funcao chamadora){

    return chamadora(vetor, tam, ordem, e_valor_para_ordem);

    // if (opcao == 1){
    //     return oe_particionamento(vetor, tam, ordem, e_valor_para_ordem);

    // }else if (opcao == 2){
    //     return oe_ordenacao(vetor, tam, ordem, e_valor_para_ordem);
    // }else{
    //     return oe_contagem(vetor, tam, ordem, e_valor_para_ordem);
    // }
}

int main(void)
{
    int ordem, valor_para_ordem;
    int vetor[] = {8,9,10,7,5,6};
    
    scanf("%d", &ordem);
    //scanf("%d", &opcao);

    if (ordem_estatistica(vetor, TAM, ordem, &valor_para_ordem, oe_ordenacao)){
        printf("A %d*-ordem estatistica tem valor %d\n", ordem, valor_para_ordem);
    }

    return 0;
}
