# PSO - Particle Swarm Optimization para Hybrid Flowshop Scheduling

## Descrição
Implementação de um algoritmo PSO (Particle Swarm Optimization) híbrido com ILS (Iterated Local Search) para resolver o problema de Hybrid Flowshop Scheduling com Preventive Maintenance.

Este projeto segue a estrutura e convenções do algoritmo genético existente, mantendo compatibilidade com:
- Leitura de instâncias no mesmo formato
- Permutações de jobs
- Decodificação com simulação de eventos
- Geração de CSV com estatísticas por geração

## Arquitetura

### Arquivos Principais

1. **scheduling_ga.h / scheduling_ga.cpp**
    - Estruturas de dados compartilhadas (Job, Machine, Event, ProblemData)
    - Funções de leitura de instâncias e permutações
    - Algoritmo de decodificação (simulação de eventos)
    - Funções de atribuição e liberação de máquinas

2. **scheduling_pso.h**
    - Estruturas PSO (Particle, GenerationStats)
    - Classe PSO com métodos de evolução

3. **scheduling_pso.cpp**
    - Implementação de operadores de crossover:
        * Order Crossover (OC)
        * Two-Point Crossover (TP)
        * PMX Crossover (PMX)
        * Position-based/Three-parent Like (PTL)
    - Implementação de operadores de mutação:
        * Swap Mutation
        * Insert Mutation
        * Multiple Swap Mutation
        * Multiple Insert Mutation (RECOMENDADO)
    - ILS-based Local Search
    - Procedimentos de aprendizado (história, local best, global best)

4. **main_pso.cpp**
    - Programa principal
    - Configuração de parâmetros
    - Execução do algoritmo

## Compilação

### Opção 1: CMake (Recomendado)
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Opção 2: Compilação Manual (Windows/MSVC)
```bash
cl /O2 /EHsc main_pso.cpp scheduling_pso.cpp scheduling_ga.cpp -o scheduling_pso.exe
```

### Opção 3: Compilação Manual (Linux/GCC)
```bash
g++ -O2 -std=c++17 main_pso.cpp scheduling_pso.cpp scheduling_ga.cpp -o scheduling_pso
```

## Uso

### Uso Básico
```bash
./scheduling_pso I1.txt generations_PSO_I1.csv
```

### Uso com Parâmetros Customizados
```bash
./scheduling_pso <instance_file> <output_file> [num_generations] [population_size]
```

Exemplo:
```bash
./scheduling_pso I1.txt generations_PSO_I1.csv 1000 150
```

## Parâmetros do PSO

Os parâmetros padrão são configurados em `main_pso.cpp`:

| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| Population Size (Ps) | 100 | Número de partículas no enxame |
| Generations | 500 | Número máximo de iterações |
| c1 | 0.2 | Coeficiente de aprendizado (local best) |
| c2 | 0.2 | Coeficiente de aprendizado (global best) |
| Mutation Probability | 0.9 | Probabilidade de mutação |
| Crossover Type | 4 (PTL) | Tipo de operador de crossover |
| Mutation Operator | 4 (MultiInsert) | Tipo de operador de mutação |

Estes valores são baseados no artigo de pesquisa e apresentam melhor desempenho.

## Saída

O programa gera um arquivo CSV com as seguintes colunas:

```csv
Generation,BestFitness,AvgFitness,WorstFitness,ElapsedTime
0,4975.0,5583.4286,6861.0,0.0274
1,4318.0,5189.2143,5461.0,0.0500
...
```

- **Generation**: Número da geração
- **BestFitness**: Melhor valor de fitness encontrado até agora
- **AvgFitness**: Fitness médio da população na geração
- **WorstFitness**: Pior fitness da população na geração
- **ElapsedTime**: Tempo decorrido em segundos

## Descrição dos Operadores

### Crossover (Pais → Filho)
- **Order Crossover (OC)**: Preserva a ordem relativa de genes
- **Two-Point Crossover (TP)**: Troca segmento entre dois pontos
- **PMX**: Partial Mapped Crossover, mantém mapeamento
- **PTL**: Position-based with Three-parent Like, combina aleatoriamente

### Mutação (Modificação in-situ)
- **Swap**: Troca dois elementos
- **Insert**: Remove um elemento e insere em outra posição
- **Multiple Swap**: Realiza múltiplas trocas
- **Multiple Insert**: Realiza múltiplas inserções (MELHOR PERFORMANCE)

### ILS Local Search
Procedimento de destruição-construção:
1. **Destruição**: Remove um elemento aleatório
2. **Construção**: Testa todas as posições possíveis e insere na melhor

## Processo de Evolução PSO

Cada partícula evolui através de três mecanismos de aprendizado:

1. **Learning from History** (Mutação)
    - Aplica operador de mutação à posição atual
    - Simula movimento aleatório

2. **Learning from Local Best** (Crossover)
    - Aplica crossover entre posição atual e melhor solução pessoal
    - Com probabilidade c1

3. **Learning from Global Best** (Crossover)
    - Aplica crossover entre posição atual e melhor solução global
    - Com probabilidade c2

Adicionalmente, a cada 5 gerações, o ILS local search é aplicado para refinar soluções.

## Estruturas de Dados

### Particle
```cpp
struct Particle {
    vector<int> position;           // Solução atual (permutação)
    vector<int> bestPosition;       // Melhor solução encontrada
    vector<int> velocity;           // Sequência de movimentos
    double fitness;                 // Fitness atual
    double bestFitness;             // Melhor fitness
};
```

### GenerationStats
```cpp
struct GenerationStats {
    int generation;
    double bestFitness;
    double avgFitness;
    double worstFitness;
    double elapsedTime;
};
```

## Formato de Instância

Arquivo de instância (ex: I1.txt):
```
10 3              # num_jobs num_stages
2 2 2             # machines_stage1 machines_stage2 machines_stage3
# Stage 1
12 28             # job1: tempo_maq1 tempo_maq2
19 39             # job2: tempo_maq1 tempo_maq2
...
# Stage 2
...
# Stage 3
...
```

## Formato de Permutação

Arquivo de permutação (ex: P1.txt):
```
4 0 9 6 8 2 7 3 5 1
```
Índices 0-based dos jobs em ordem de processamento.

## Função Objetivo

O algoritmo minimiza o **Total Tardiness** (Atraso Total):

```
Minimizar: Σ(Tj) para j = 1, ..., n
Tj = max(0, Cij_final - dj)
```

Onde:
- Tj: Tardiness do job j
- Cij_final: Tempo de conclusão do job j no último estágio
- dj: Due date (prazo) do job j

## Comparação com GA

| Aspecto | GA | PSO |
|---------|----|----|
| Populacao | Cromossomos | Partículas |
| Variação | Crossover + Mutação | Mutação + Crossover |
| Seleção | Baseada em fitness | Informação social (local/global best) |
| Local Search | Não (padrão) | Sim (ILS) |
| Convergência | Gradual | Rápida |

## Dicas de Uso

1. **Para problemas pequenos** (< 20 jobs):
    - Reduzir população para 50
    - Aumentar gerações para 2000

2. **Para problemas médios** (20-50 jobs):
    - Usar população 100-150
    - Gerações 500-1000

3. **Para problemas grandes** (> 50 jobs):
    - Aumentar população para 200+
    - Usar gerações 500+

4. **Melhorar qualidade da solução**:
    - Aumentar probabilidade de mutação (até 0.95)
    - Aplicar ILS mais frequentemente (a cada geração)
    - Aumentar população

5. **Acelerar execução**:
    - Reduzir número de gerações
    - Reduzir população
    - Reduzir frequência de ILS

## Validação

Para validar os resultados, compare com:
- Valores de benchmark conhecidos
- Resultados do algoritmo genético
- Outros algoritmos (IG, ILS, hGA)

## Referências

Paper utilizado como base:
- "A Hybrid PSO-ILS Algorithm for Hybrid Flowshop Scheduling with Preventive Maintenance"
- Operadores e parâmetros otimizados conforme artigo

## Licença

Este projeto é fornecido como material de pesquisa e educação.

## Autor

Implementação em C++ do algoritmo PSO conforme especificação do artigo de pesquisa.
Data: 2025
