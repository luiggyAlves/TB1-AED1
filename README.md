# TB1 — Estatísticas de Ordem: Qualidade do Ar e Clima de Manaus

Trabalho 1 de ICC002 — Algoritmos e Estruturas de Dados I (UFAM/IComp, 2026/2).
Especificação: [`especificacao_trabalho1.pdf`](especificacao_trabalho1.pdf).

O programa encontra o k-ésimo menor elemento de um conjunto de medições climáticas por três
abordagens, baseadas no código da disciplina ([`referencia/ordem__ultimaversao.c`](referencia/ordem__ultimaversao.c)):

* **A**: ordenação completa pela bolha e acesso direto;
* **B**: seleção por particionamento com pivô central;
* **C**: contagem comparativa sequencial.

Com elas, o programa responde à mediana e ao percentil 90 do PM2.5 e lista os M dias mais
secos, contando as comparações de cada abordagem.

**Relatório técnico:** [`RELATORIO.md`](RELATORIO.md).

## Como compilar e executar

```sh
make            # compila build/ordem_estatistica
make run        # executa com dados/dados_clima_manaus.csv e grava resultados/saida.txt
make test       # testes contra uma referência por força bruta
```

Execução manual:

```sh
./build/ordem_estatistica [arquivo.csv] [-n N] [-m M] [-s semente] [-o saida.csv]
```

| Opção | Padrão | Significado |
|---|---|---|
| `arquivo.csv` | `dados/dados_clima_manaus.csv` | medições no formato `ID_Sensor,Data,Temperatura_C,Umidade_Porcentagem,Nivel_PM25` |
| `-n N` | 10000 | tamanho mínimo do conjunto; se o arquivo tiver menos registros, o restante é simulado |
| `-m M` | 10 | quantidade de dias mais secos a listar |
| `-s semente` | 2026 | semente da simulação (resultados reprodutíveis) |
| `-o arquivo` | `resultados/dados_utilizados.csv` | onde gravar o conjunto efetivamente usado (`-` para não gravar) |

Requer apenas um compilador C11 (gcc ou clang) e `make`.

## Estrutura

```
src/ordem_estatistica.c/.h   abordagens A, B e C + despachante ordem_estatistica (t_funcao)
src/medicao.c/.h             registro Medicao, leitura/gravação de CSV, simulação
src/aleatorio.c/.h           gerador pseudoaleatório reprodutível (SplitMix64)
src/experimentos.c/.h        consultas, tabela de desempenho, melhor/pior caso, questões 1–5
src/main.c                   programa principal
tests/testes.c               testes
dados/                       CSV fornecido
resultados/                  saída da execução e conjunto de dados utilizado
referencia/                  código-base fornecido pelo professor
```
