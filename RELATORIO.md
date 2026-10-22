# Relatório Técnico — Trabalho 1

**ICC002 — Algoritmos e Estruturas de Dados I — 2026/2**
**Análise da Qualidade do Ar e Clima de Manaus através de Estatísticas de Ordem**

Todos os números deste relatório vêm da execução de `make run`. A saída completa está em
[`resultados/saida.txt`](resultados/saida.txt) e o conjunto de dados usado em
[`resultados/dados_utilizados.csv`](resultados/dados_utilizados.csv). As contagens de
comparações são determinísticas (semente fixa). Os tempos variam de uma máquina para outra.

---

## 1. Visão geral da solução

| Arquivo | Conteúdo |
|---|---|
| `src/ordem_estatistica.c/.h` | As três abordagens (A, B, C) e o despachante `ordem_estatistica` |
| `src/medicao.c/.h` | Registro `Medicao`, leitura/gravação do CSV, simulação dos dados |
| `src/aleatorio.c/.h` | Gerador pseudoaleatório próprio (SplitMix64), para resultados reprodutíveis |
| `src/experimentos.c/.h` | Consultas, tabela de desempenho, melhor/pior caso e questões 1 a 5 |
| `src/main.c` | Programa principal (argumentos, entrada dos dados, chamada das seções) |
| `tests/testes.c` | Testes das três abordagens contra uma referência por força bruta |
| `referencia/ordem__ultimaversao.c` | Código-base fornecido pelo professor |

Nenhuma função pronta de ordenação ou seleção da biblioteca padrão é usada (não há
`qsort`, por exemplo). A única "ordenação" do programa é a própria bolha da Abordagem A.

### 1.1 Relação com o código-base do professor

As funções mantêm os nomes e a estrutura de `ordem__ultimaversao.c`: `trocar`, `bolha`,
`oe_ordenacao`, `contagem`, `oe_contagem`, `particionamento` (com **vetor espelho** e
**pivô central** `(i+j)/2`), `oe_particionamento`, o tipo ponteiro de função `t_funcao` e o
despachante `ordem_estatistica(vetor, tam, ordem, &valor, chamadora)`. As adaptações foram:

1. **Vetor de registros em vez de `int`.** O vetor é de `Medicao`
   (`ID_Sensor`, `Data`, `Temperatura_C`, `Umidade_Porcentagem`, `Nivel_PM25`). O campo
   usado como chave (PM2.5, umidade ou temperatura) é escolhido por um `Contexto`, que
   também conta as comparações.
2. **Contagem com empates (Abordagem C).** O código-base contava só os elementos
   estritamente menores (`<`) e aceitava o elemento quando `menores + 1 == ordem`. Com
   valores repetidos isso entra em laço infinito ou lê fora do vetor (ver Questão 2). A
   versão corrigida conta também os **menores ou iguais**, como pede a especificação, e
   aceita o elemento se `menores < ordem <= menores_ou_iguais`.
3. **Particionamento com empates (Abordagem B).** No código-base, os elementos iguais ao
   pivô (fora o próprio pivô) não eram copiados para o vetor espelho. As posições
   `i..j-1` ficavam com lixo e dados eram perdidos. Agora todos os iguais ocupam o bloco
   `i..j`, e o laço externo usa esse bloco inteiro: se `ordem` cair nele, a resposta foi
   encontrada. Caso contrário, a busca continua só à esquerda (`j_lim = i-1`) ou só à
   direita (`i_lim = j+1`).
4. **Vetor espelho alocado uma única vez** com `malloc`, em vez de um VLA na pilha a cada
   partição, o que evita estouro de pilha com N grande.
5. A função `particionamento_semvetoraux` do código-base **não foi usada**. Quando
   `vetor[i] == vetor[j] == pivo` com `i < j`, ela troca os dois elementos sem avançar
   `i` nem `j` e entra em laço infinito. Valores repetidos são muito comuns nesses dados.

A bolha foi mantida **exatamente** como no código-base: `tam-1` passadas completas, sem
parada antecipada.

### 1.2 Métrica de custo

**Uma comparação é um confronto entre as chaves de dois registros.** A função
`comparar()` responde "menor", "igual" ou "maior" e incrementa o contador uma vez.
Comparações entre índices e contadores não contam. Com essa métrica:

* a bolha faz uma comparação por par adjacente examinado;
* o particionamento faz uma comparação por elemento do subvetor (incluindo o pivô);
* a contagem faz uma comparação por par (elemento avaliado, elemento do vetor). Como no
  código-base, o elemento avaliado também é comparado com ele mesmo.

---

## 2. Dados de entrada

O CSV fornecido (`dados/dados_clima_manaus.csv`) tem **10 registros** reais. A
especificação exige N ≥ 10.000, e permite *"simular ou receber um arquivo"*. Por isso o
programa lê o CSV e **completa com 9.990 registros simulados**, chegando a N = 10.000. Os
10 registros reais são os 10 primeiros do conjunto.

O leitor de CSV aceita cabeçalho, finais de linha LF/CRLF e descarta linhas malformadas.
O PM2.5 é lido como `float`, conforme a especificação, mesmo vindo inteiro no arquivo.

**Modelo de simulação** (`simular_medicoes`, período 01/08 a 31/10/2026):
* cada dia tem uma intensidade de fumaça `f` que cresce até o fim de setembro (auge das
  queimadas) e tem episódios aleatórios de "fumaceiro" que podem triplicá-la;
* temperatura: normal em torno de 30,5 °C + 1,2(f−1), truncada em [24, 40] °C;
* umidade: inversamente relacionada à temperatura e à fumaça, truncada em [12, 98] %;
* PM2.5: log-normal `16·f·e^(0,45z)`, entre 3 e 450 µg/m³, com uma casa decimal.

Os valores têm uma casa decimal (temperatura, PM2.5) ou são inteiros (umidade). Por isso
há **muitos valores repetidos**, o que torna obrigatório tratar empates corretamente.
Opções: `-n N` (tamanho), `-m M` (dias secos), `-s semente`, `-o arquivo` (onde gravar o
conjunto usado).

---

## 3. Resultados das consultas obrigatórias (N = 10.000)

| Consulta | k | Resposta | A (comparações) | B (comparações) | C (comparações) |
|---|---:|---|---:|---:|---:|
| Mediana do PM2.5 | 5.000 | **33,3 µg/m³** | 49.995.000 | 41.548 | 2.510.000 |
| Percentil 90 do PM2.5 (limiar crítico) | 9.000 | **85,4 µg/m³** | 49.995.000 | 38.231 | 84.450.000 |
| 10 dias mais secos (umidade) | M = 10 | **12 %** (todos os 10) | 49.995.000 | 10.247 | 58.130.000 |

* As três abordagens retornam sempre o mesmo valor (o programa confere e informa).
* **Mediana:** k = N/2 = 5.000 (com N ímpar usa-se ⌈N/2⌉, a mediana exata).
* **Percentil 90:** k = ⌈0,9·N⌉ = 9.000 (definição por posto mais próximo). Exatamente
  **1.000 medições (10 %)** ficam acima de 85,4 µg/m³: são os dias em alerta severo.
* **Dias mais secos:** a listagem completa (data, sensor, umidade, temperatura, PM2.5)
  está na seção 2 de `resultados/saida.txt`. Com umidades repetidas, abordagens diferentes
  podem escolher dias distintos **com a mesma umidade**, e o programa confere os valores.
  * A: ordena e pega `vetor[0..M-1]`.
  * B: seleciona o M-ésimo menor, e o particionamento deixa os M menores em
    `vetor[0..M-1]`.
  * C: varre os elementos e, com a mesma contagem, guarda os que ocupam uma das M
    primeiras posições (`listar_menores_contagem`). Ela trata o caso de um valor repetido
    "na fronteira" da M-ésima posição.

---

## 4. Análise de desempenho (tabela comparativa)

Para cada N foram usados os N primeiros registros do conjunto.

| N | Abordagem | Mediana | P90 | 10 mais secos | Tempo total (ms)* |
|---:|---|---:|---:|---:|---:|
| 1.000 | A (bolha) | 499.500 | 499.500 | 499.500 | 10,4 |
| 1.000 | B (particionamento) | 2.697 | 2.446 | 1.753 | 0,09 |
| 1.000 | C (contagem) | 200.000 | 541.000 | 915.000 | 11,7 |
| 5.000 | A (bolha) | 12.497.500 | 12.497.500 | 12.497.500 | 290,9 |
| 5.000 | B (particionamento) | 23.339 | 14.922 | 9.218 | 0,43 |
| 5.000 | C (contagem) | 1.775.000 | 2.415.000 | 22.925.000 | 204,9 |
| 10.000 | A (bolha) | 49.995.000 | 49.995.000 | 49.995.000 | 1.200,8 |
| 10.000 | B (particionamento) | 41.548 | 38.231 | 10.247 | 0,77 |
| 10.000 | C (contagem) | 2.510.000 | 84.450.000 | 58.130.000 | 1.127,0 |

\* Tempo de CPU somando as três consultas. Valores de uma execução de referência; o
número de comparações é a medida pedida e não depende da máquina.

**Crescimento de N = 1.000 para N = 10.000** (N 10 vezes maior), somando as três
consultas:

| Abordagem | Comparações | Tempo |
|---|---:|---:|
| A (bolha) | × 100,1 | ≈ × 115 |
| B (particionamento) | × 13,1 | ≈ × 9 |
| C (contagem) | × 87,6 | ≈ × 96 |

Crescimento linear dá ≈ ×10, e quadrático dá ≈ ×100. B cresce de forma linear. A e C
crescem de forma quadrática.

---

## 5. Melhor e pior caso de cada algoritmo

### 5.1 Análise teórica (n = tamanho do vetor, k = ordem buscada)

| Abordagem | Melhor caso | Pior caso | Caso médio |
|---|---|---|---|
| **A — bolha + acesso direto** | n(n−1)/2 → Θ(n²) | n(n−1)/2 → Θ(n²) | n(n−1)/2 |
| **B — particionamento** | n → Θ(n) | Σ<sub>t=0..k−1</sub>(n−t) = kn − k(k−1)/2 → Θ(n²) (máx. n(n+1)/2 com k = n) | ≈ 3,4n → Θ(n) |
| **C — contagem** | n → Θ(n) | n² → Θ(n²) | ≈ n²/2 (valores distintos) |

**A (bolha).** O laço interno percorre `tam-1-j` pares na passada `j`, então o total é
(n−1) + (n−2) + … + 1 = n(n−1)/2, **independentemente dos dados**. A bolha do código-base
não interrompe quando uma passada fica sem trocas, então melhor e pior caso são iguais em
comparações. O que muda é o número de **trocas**: 0 com o vetor já ordenado e n(n−1)/2
com o vetor em ordem decrescente. Depois de ordenar, o acesso a `vetor[k-1]` custa 0
comparações.

**B (particionamento).** Cada chamada de `particionamento` sobre um subvetor de s
elementos faz exatamente s comparações.
* *Melhor caso:* o primeiro pivô já é o k-ésimo (ou o bloco de iguais ao pivô contém a
  posição k), com **n comparações**. Exemplos: vetor já ordenado com pivô central e k =
  mediana, ou todos os valores iguais.
* *Pior caso:* em toda etapa o pivô é o extremo do subvetor do lado oposto a k, então
  cada etapa descarta só um elemento: n + (n−1) + … Para k = mediana, isso dá ≈ 3n²/8.
  Para k = n, dá n(n+1)/2. É quadrático, como a bolha.
* *Caso médio:* cada partição descarta em média uma fração constante do vetor, então o
  total é uma série geométrica, O(n). Para a mediana de dados aleatórios, a média
  conhecida é 2(1 + ln 2)·n ≈ 3,39n. Uma medição extra com 200 permutações aleatórias deu
  **3,35n** (n = 1.000) e **3,30n** (n = 10.000).

**C (contagem).** Avaliar um elemento custa n comparações (ele contra todos). O algoritmo
avalia os elementos em sequência até achar o primeiro que ocupa a posição k.
* *Melhor caso:* o primeiro elemento já é a resposta, com **n comparações**.
* *Pior caso:* só o último elemento é a resposta, então todos os n são avaliados: **n²
  comparações**.
* *Caso médio:* com valores distintos, a resposta está numa posição uniforme e o custo
  médio é n(n+1)/2 ≈ n²/2 (medido: 0,51n² e 0,53n²). Se o valor procurado tem c cópias, a
  primeira cópia aparece em média na posição (n+1)/(c+1), e o custo cai para
  ≈ n²/(c+1). É por isso que a mediana real custou "só" 2,5 milhões (a primeira cópia de
  33,3 estava na posição 251), enquanto o P90 custou 84,5 milhões (posição 8.445).
* *Dias mais secos:* a varredura só termina quando os M registros forem encontrados, então
  no pior caso também custa n².

### 5.2 Melhor e pior caso medidos (k = mediana)

Cenários com chaves 1..n montados pelo programa (seção 4 da saída). O cenário
"adversário do pivô central" é gerado simulando o próprio particionamento: o elemento
que cai no meio do subvetor recebe sempre o menor valor ainda livre.

**N = 1.000 (k = 500)**

| Cenário | A (bolha) | B (partição) | C (contagem) |
|---|---:|---:|---:|
| aleatório, valores distintos | 499.500 | 7.072 | 539.000 |
| já ordenado (crescente) | 499.500 | **1.000** (melhor) | 500.000 |
| ordem decrescente | 499.500 | 1.989 | 501.000 |
| todos os valores iguais | 499.500 | **1.000** (melhor) | **1.000** (melhor) |
| resposta na 1ª posição | 499.500 | 3.042 | **1.000** (melhor) |
| resposta na última posição | 499.500 | 3.959 | **1.000.000** (pior = n²) |
| adversário do pivô central | 499.500 | **375.250** (pior) | 750.000 |

**N = 10.000 (k = 5.000)**

| Cenário | A (bolha) | B (partição) | C (contagem) |
|---|---:|---:|---:|
| aleatório, valores distintos | 49.995.000 | 47.527 | 52.990.000 |
| já ordenado (crescente) | 49.995.000 | **10.000** | 50.000.000 |
| ordem decrescente | 49.995.000 | 19.987 | 50.010.000 |
| todos os valores iguais | 49.995.000 | **10.000** | **10.000** |
| resposta na 1ª posição | 49.995.000 | 31.994 | **10.000** |
| resposta na última posição | 49.995.000 | 45.819 | **100.000.000** |
| adversário do pivô central | 49.995.000 | **37.502.500** | 75.000.000 |

Os valores medidos coincidem exatamente com as fórmulas:
* A = n(n−1)/2 em todos os cenários;
* melhor caso de B e de C = n;
* pior caso de C = n²;
* pior caso de B = kn − k(k−1)/2: 375.250 para n = 1.000 e 37.502.500 para n = 10.000.

---

## 6. Discussão: Contagem Comparativa (C) × Particionamento (B) em grandes volumes

As duas abordagens não modificam o vetor de forma completa e podem parar assim que
encontram a resposta. A diferença está em **quanto trabalho cada passo elimina**:

* **B** compara cada elemento do subvetor uma vez com o pivô e **descarta de uma vez**
  toda a parte que não pode conter a resposta. Em média, cada etapa elimina uma fração
  constante dos dados. O custo total é n + n/2 + n/4 + … = O(n). Com N = 10.000, as
  consultas reais custaram entre 10 mil e 42 mil comparações, menos de 1 ms.
* **C** gasta n comparações para descobrir a posição de **um único** elemento, e essa
  informação **não é reaproveitada** para os demais: o próximo elemento recomeça a
  contagem do zero. São dois laços aninhados sobre n, com custo médio ≈ n²/2. Com
  N = 10.000, o P90 exigiu 84,45 milhões de comparações, **mais de 2.200 vezes** as
  38.231 de B, e foi mais caro até que a bolha (49,99 milhões).

**Projeção para os volumes de Manaus.** A taxa medida foi de cerca de 1,3 × 10⁸
comparações por segundo (84,45 milhões em ≈ 0,63 s). Com isso:

| N | B (≈ 3,4n) | C médio (≈ n²/2) | C pior (n²) |
|---:|---:|---:|---:|
| 10⁴ | 3,4 × 10⁴ (< 1 ms) | 5 × 10⁷ (≈ 0,4 s) | 10⁸ (≈ 0,8 s) |
| 10⁶ | 3,4 × 10⁶ (≈ 30 ms) | 5 × 10¹¹ (≈ 1 hora) | 10¹² (≈ 2 horas) |
| 10⁸ | 3,4 × 10⁸ (≈ 3 s) | 5 × 10¹⁵ (≈ 1,2 ano) | 10¹⁶ (≈ 2,4 anos) |

Multiplicar N por 10 multiplica o custo de B por ≈ 10 e o de C por ≈ 100 (medido: ×13,1
contra ×87,6). Para milhares de sensores enviando dados continuamente, só B (seleção
linear) é viável. C só é razoável para conjuntos pequenos ou como instrumento didático.
Ela tem duas vantagens pontuais: não altera o vetor e fica rápida quando o valor
procurado se repete muito e aparece cedo, como a mediana real (2,5 milhões).

---

## 7. Questões de acompanhamento prático

### Questão 1 — Rastreando a Abordagem C

Lista `[45, 90, 30, 90, 60]`, mediana k = 3. Cada elemento avaliado é comparado com os 5
elementos. Ele ocupa no vetor ordenado as posições `menores+1 .. menores_ou_iguais`:

| Passo | Elemento avaliado | Menores (<) | Menores ou iguais (≤) | Posições que ocupa | k = 3? | Comparações |
|---:|---:|---:|---:|---|---|---:|
| 1 | 45 | 1 (30) | 2 | 2..2 | não | 5 |
| 2 | 90 | 3 (45, 30, 60) | 5 | 4..5 | não | 5 |
| 3 | 30 | 0 | 1 | 1..1 | não | 5 |
| 4 | 90 | 3 | 5 | 4..5 | não | 5 |
| 5 | 60 | 2 (45, 30) | 3 | **3..3** | **sim → 60** | 5 |

A mediana é **60**. Ela é o último elemento da lista, então todos os elementos foram
avaliados: **5 × 5 = 25 comparações diretas** no total. O rastreio linha a linha de
cada comparação está na seção 5.1 da saída do programa. Se o elemento não fosse comparado
consigo mesmo, seriam 5 × 4 = 20.

### Questão 2 — Empates (medições iguais)

Com três dias de pico idêntico, `[45, 90, 30, 90, 90]`, a lista ordenada é
`[30, 45, 90, 90, 90]`.

* **Contando só os estritamente menores (`<`):** cada 90 tem 2 menores e recebe a
  "ordem" 3. As ordens **4 e 5 não são atribuídas a nenhum elemento**, então o critério
  `menores + 1 == ordem` nunca é satisfeito para k = 4 ou 5. No código-base, o `do-while`
  incrementa `i` indefinidamente e passa a ler `vetor[5]`, `vetor[6]`, … fora do vetor.
  Isso é comportamento indefinido: lixo como resposta, laço que não termina ou falha de
  segmentação. O programa demonstra isso na seção 5.2 com uma versão protegida por limite,
  que informa "NÃO ENCONTRA" para k = 4 e 5.
* **Ajuste feito:** a função `contagem` devolve dois contadores, `menores` (`<`) e
  `menores_ou_iguais` (`≤`). Um valor com c cópias ocupa um **intervalo** de posições,
  `menores+1 .. menores_ou_iguais`, e não uma posição só. O critério virou
  `menores < k <= menores_ou_iguais`: qualquer cópia do 90 responde corretamente a k = 3,
  4 ou 5. Como toda posição 1..n pertence ao intervalo de algum elemento, o laço sempre
  termina. Por segurança ele também tem o limite `i < tam`. A mesma ideia de intervalo foi
  aplicada ao particionamento (bloco `i..j` de iguais ao pivô) e à listagem dos M dias
  mais secos.

### Questão 3 — Escolha do pivô (Abordagem B)

Experimento da seção 5.3: 10.000 registros reais de PM2.5, primeiro na ordem original e
depois **já ordenados do menos ao mais poluído**.

| Entrada | Pivô | Mediana | P90 |
|---|---|---:|---:|
| ordem original | primeiro elemento | 44.325 | 12.662 |
| ordem original | aleatório | 71.018 | 32.092 |
| ordem original | central (exigido) | 41.548 | 38.231 |
| **já ordenada** | **primeiro elemento** | **9.906.608** | **6.029.190** |
| já ordenada | aleatório | 26.970 | 35.069 |
| já ordenada | central (exigido) | 10.000 | 19.930 |

* **Dados ordenados + pivô no primeiro elemento:** o pivô é sempre o **menor** valor do
  subvetor. A partição fica com zero elementos à esquerda e todo o resto à direita, e só o
  pivô (e suas cópias) é descartado. Para chegar à mediana são necessárias milhares de
  partições quase do tamanho do vetor: n + (n−1) + (n−2) + … O custo vira **quadrático**
  (9,9 milhões de comparações contra 10 mil) e o algoritmo perde a vantagem sobre a bolha.
* **Pivô aleatório:** a lentidão vem de o pivô ser *sempre* o extremo. Com sorteio, a
  chance de cair repetidamente num extremo é mínima, qualquer que seja a ordem de entrada:
  em média o pivô cai "no meio" e descarta uma fração constante do vetor a cada etapa. O
  custo esperado é linear (≈ 3,4n) para **qualquer** entrada. Nenhuma organização prévia
  dos dados, nem mesmo um arquivo já ordenado por data ou por poluição, provoca o pior
  caso de forma sistemática.
* O pivô central exigido no trabalho também se sai muito bem com dados ordenados (é o
  melhor caso: 10.000 comparações). Ainda assim ele tem uma entrada "adversária"
  determinística (seção 5.2), que custou 37,5 milhões de comparações. O pivô aleatório
  não tem uma entrada fixa que o derrube.

### Questão 4 — O gargalo na prática

A abordagem que mais sofreu foi a **C (contagem comparativa)**, seguida da A. De
N = 1.000 para N = 10.000 (10× mais dados), as comparações de C cresceram **×87,6** e o
tempo ≈ ×96. No P90 com N = 10.000, C fez 84,45 milhões de comparações, mais que a bolha.
B cresceu só ×13.

O motivo está na estrutura de **laços dentro de laços**:

```c
do {                                   /* laço externo: até n elementos avaliados */
    contagem(vetor, tam, &vetor[i], …);  /* laço interno: n comparações cada */
    …
} while (!achei && i < tam);
```

Para cada um dos até n candidatos, o laço interno percorre os n elementos, totalizando
até n × n = n² comparações. Dobrar o volume quadruplica o trabalho e multiplicá-lo por
1.000 o multiplica por 1.000.000. Para milhões de medições por segundo, C precisaria de
≈ 10¹² comparações para **cada** consulta sobre 10⁶ valores (horas de processamento),
enquanto os dados continuariam chegando. Além disso, nada do que é contado para um
elemento é reaproveitado para o próximo.

### Questão 5 — Reutilização × velocidade única

Experimento da seção 5.5 (N = 10.000): primeiro a mediana, depois o P90.

| Estratégia | Mediana | P90 | Total |
|---|---:|---:|---:|
| A: ordena 1 vez + acesso direto | 49.995.000 | **0** | 49.995.000 |
| B: novo particionamento (mesmo vetor) | 41.548 | 19.949 | 61.497 |
| B: P90 sobre uma cópia nova | — | 38.231 | — |

* **A não precisa ordenar de novo.** Depois da ordenação completa, o vetor fica
  totalmente organizado e **qualquer** ordem estatística (P90, quartis, máximo, os M mais
  secos) sai por acesso direto a `vetor[k-1]`, com 0 comparações. O custo alto
  (≈ 50 milhões) é pago uma vez e reaproveitado.
* **B não se comporta assim.** O particionamento só garante a posição do elemento
  procurado e a separação "menores à esquerda / maiores à direita". O resto do vetor
  continua desorganizado, então **cada nova pergunta exige um novo particionamento**. A
  organização parcial deixada pela primeira consulta ajudou um pouco nesse caso (19.949
  contra 38.231 numa cópia nova), mas não dispensa o trabalho.
* **Diferença prática:** B é imbatível para uma pergunta isolada, ou para poucas. Duas
  consultas custaram 61 mil comparações, cerca de 800× menos que a bolha. A ordenação
  completa só compensa quando o número de consultas é muito grande. Com a bolha, isso
  exigiria mais de mil consultas sobre os mesmos dados, porque cada seleção por B custa
  ≈ 40 mil comparações contra 50 milhões da bolha. Com um algoritmo de ordenação
  O(n log n), esse ponto de equilíbrio cairia bastante.

---

## 8. Verificação da conformidade com a especificação

| Requisito | Situação |
|---|---|
| Entrada por arquivo com N ≥ 10.000 registros (simular ou receber) | ✔ Lê o CSV fornecido (10 registros) e simula o restante até N = 10.000. Aceita CSVs maiores; `-n` ajusta N |
| Campos ID_Sensor (int), Data (string), Temperatura_C (float), Umidade_Porcentagem (int), Nivel_PM25 (float) | ✔ `struct Medicao` com exatamente esses tipos |
| Abordagem A: ordenar todo o vetor com a **bolha** e acessar a posição k | ✔ `bolha` + `oe_ordenacao` (código-base) |
| Abordagem B: seleção por particionamento com **pivô centralizado**, reduzindo o espaço de busca | ✔ `particionamento` (vetor espelho, pivô `(i+j)/2`) + `oe_particionamento` |
| Abordagem C: contagem sequencial de quantos são **menores ou iguais**, verificando o critério da k-ésima posição | ✔ `contagem` (`<` e `≤`) + `oe_contagem`, com critério `menores < k ≤ menores_ou_iguais` |
| Consulta: mediana do PM2.5 (k = N/2) | ✔ 33,3 µg/m³ pelas três abordagens |
| Consulta: percentil 90 do PM2.5 | ✔ 85,4 µg/m³ (1.000 dias acima) |
| Consulta: listar os M dias mais secos | ✔ M = 10 (ajustável com `-m`), listados com data, sensor e demais campos |
| Medir comparações de A, B e C para N = 1.000, 5.000 e 10.000 | ✔ Seção 3 da saída e seção 4 deste relatório |
| Tabela comparativa | ✔ |
| Código em C, sem funções prontas de seleção/ordenação | ✔ Sem `qsort` ou similares. Compila sem avisos com `-std=c11 -Wall -Wextra -pedantic` |
| Relatório: melhor e pior caso de cada algoritmo | ✔ Seção 5 (teórico e medido) |
| Relatório: custo de C frente a B para grandes volumes | ✔ Seção 6 |
| Questões de acompanhamento 1 a 5 | ✔ Seção 7, com demonstrações executáveis na seção 5 da saída |

**Testes.** `make test` executa mais de 8 milhões de verificações:
* as três abordagens (e as três estratégias de pivô) são conferidas contra uma referência
  por força bruta em 400 vetores aleatórios com muitos valores repetidos, para todo k;
* nenhum registro é perdido ou duplicado;
* B deixa os k menores à esquerda;
* A ordena e faz n(n−1)/2 comparações;
* a listagem dos M menores está correta;
* a Questão 1 dá 60 com 25 comparações.

O programa e os testes também foram executados com AddressSanitizer e
UndefinedBehaviorSanitizer, sem nenhum erro.
